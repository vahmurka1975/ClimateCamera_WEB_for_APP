// Versioned, CRC-checked local backups. Passwords in backups are NOT encrypted.
#pragma once
#include <memory>
#include <new>
static constexpr uint32_t BACKUP_MAGIC=0x43434234, TX_MAGIC=0x54583134;
static constexpr uint32_t BK_PRODUCTS=1,BK_SETTINGS=2;
struct BackupHeader {uint32_t magic,format,flags,length,configSize,deviceSize,productsSize,crc;};
struct BackupSettings {Config climate;DeviceSettings network;};
struct TransactionHeader {uint32_t magic,flags,length,crc;};
static bool configValidImage(const Config &c) {
  const uint8_t *raw=(const uint8_t*)&c;
  if(raw[offsetof(Config,enabled)]>1||raw[offsetof(Config,humidEnabled)]>1||raw[offsetof(Config,dehumEnabled)]>1)return false;
#define RANGE(f,a,b) if(!isfinite(c.f)||c.f<a||c.f>b)return false;
  RANGE(tempTarget,2,30) RANGE(tempHyst,.2f,5) RANGE(humTarget,30,98) RANGE(humHyst,1,10)
  RANGE(tempOffset,-5,5) RANGE(humOffset,-15,15) RANGE(pidKp,0,1) RANGE(pidKi,0,1)
#undef RANGE
  return c.fanWorkMin>=1&&c.fanWorkMin<=180&&c.fanPauseMin<=360&&c.fanTargetRPM>=300&&c.fanTargetRPM<=1500;
}
static bool realDate(const char *d,size_t cap) {
  if(strnlen(d,cap)!=10||d[4]!='-'||d[7]!='-')return false;
  for(int i=0;i<10;i++)if(i!=4&&i!=7&&(d[i]<'0'||d[i]>'9'))return false;
  int y=atoi(d),m=atoi(d+5),day=atoi(d+8);if(y<1970||y>9999||m<1||m>12)return false;
  int days[]={31,28,31,30,31,30,31,31,30,31,30,31};if(y%4==0&&(y%100!=0||y%400==0))days[1]=29;
  return day>=1&&day<=days[m-1];
}
static bool productsValidImage(const ProductSlot *slots) {
  for(int i=0;i<MAX_PRODUCTS;i++) {
    const ProductSlot &p=slots[i];if(*(const uint8_t*)&p>1)return false;if(!p.active)continue;
    if(!isfinite(p.initialWeight)||p.initialWeight<=0||p.initialWeight>1e7f||!isfinite(p.targetLossPct)||p.targetLossPct<5||p.targetLossPct>80||p.count>MAX_WEIGHINGS||!realDate(p.startDate,sizeof(p.startDate)))return false;
    for(int j=0;j<p.count;j++)if(!realDate(p.history[j].date,sizeof(p.history[j].date))||!isfinite(p.history[j].weight)||p.history[j].weight<=0||p.history[j].weight>1e7f)return false;
  }return true;
}
static bool sameSecret(const String &input,const char *expected) {
  size_t len=strlen(expected);uint32_t diff=(uint32_t)(input.length()^len);
  for(size_t i=0;i<len;i++)diff|=(uint8_t)expected[i]^(uint8_t)(i<input.length()?input[i]:0);return diff==0;
}
static uint8_t sensitiveFails=0;static uint32_t sensitiveLockedUntil=0;
static bool sensitiveAccess(const char *confirmation) {
  if(!authorized(true))return false;
  if(sensitiveFails>=5&&(int32_t)(sensitiveLockedUntil-millis())>0){sendMessage(429,false,"Too many administrator password attempts; wait 60 seconds");return false;}
  if(!sameSecret(server.header("X-Admin-Password"),device.adminPass)) {
    if(++sensitiveFails>=5)sensitiveLockedUntil=millis()+60000;sendMessage(403,false,"Administrator password required");return false;
  }
  sensitiveFails=0;
  if(server.header("X-Climate-Confirm")!=confirmation){sendMessage(400,false,"Explicit confirmation required");return false;}
  CameraSnapshot state={};if(!cameraReadSnapshot(state)||state.otaMode||state.uiEditing){sendMessage(409,false,"Finish OTA/local editing first");return false;}
  return true;
}
static size_t payloadLength(uint32_t flags) {return ((flags&BK_SETTINGS)?sizeof(BackupSettings):0)+((flags&BK_PRODUCTS)?sizeof(products):0);}
static bool payloadValid(uint32_t flags,const uint8_t *data,size_t size) {
  if(!flags||(flags&~3U)||size!=payloadLength(flags))return false;size_t offset=0;
  if(flags&BK_SETTINGS){BackupSettings image={};memcpy(&image,data,sizeof(image));if(!configValidImage(image.climate)||!deviceValid(image.network))return false;offset+=sizeof(image);}
  return !(flags&BK_PRODUCTS)||productsValidImage((const ProductSlot*)(data+offset));
}
static bool executeTransaction(uint32_t flags,const uint8_t *data,size_t length) {
  if(!payloadValid(flags,data,length))return false;size_t offset=0;
  if(flags&BK_SETTINGS){BackupSettings settings={};memcpy(&settings,data,sizeof(settings));settings.climate.enabled=false;
    CameraCommand cmd={};cmd.kind=CameraCommandKind::RestoreConfig;cmd.config=settings.climate;CameraReply reply={};
    if(!cameraSubmitCommand(cmd,reply,5000)||!reply.ok||!saveDevice(settings.network))return false;
    device=settings.network;offset+=sizeof(settings);
  }
  if(flags&BK_PRODUCTS){auto slots=(const ProductSlot*)(data+offset);if(!saveProductsImage(slots))return false;memcpy(products,slots,sizeof(products));productsLoaded=true;}
  return true;
}
static bool recoverAdminTransaction() {
  auto part=managementPartition();if(!part)return false;TransactionHeader h={};
  if(esp_partition_read(part,0x10000,&h,sizeof(h))!=ESP_OK)return false;
  if(h.magic!=TX_MAGIC)return true;
  adminTransactionBlocked=true;
  if(h.length!=payloadLength(h.flags)||h.length>0xf000||!h.flags||(h.flags&~3U))return false;
  auto image=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[h.length]);if(!image)return false;
  if(esp_partition_read(part,0x10000+sizeof(h),image.get(),h.length)!=ESP_OK||h.crc!=dataCrc(image.get(),h.length)||!executeTransaction(h.flags,image.get(),h.length))return false;
  if(esp_partition_erase_range(part,0x10000,0x10000)!=ESP_OK)return false;adminTransactionBlocked=false;return true;
}
static bool commitTransaction(uint32_t flags,const uint8_t *data,size_t length) {
  if(!payloadValid(flags,data,length))return false;
  // All validation precedes outputs-off. Persist the manual-start interlock before any flash updates.
  CameraCommand off={};off.kind=CameraCommandKind::SystemOff;CameraReply reply={};if(!cameraSubmitCommand(off,reply,5000)||!reply.ok)return false;
  Preferences boot;if(!boot.begin("climate",false))return false;bool saved=boot.putBool("boot_off",true)==1;boot.end();if(!saved)return false;
  auto part=managementPartition();if(!part)return false;
  TransactionHeader h={TX_MAGIC,flags,(uint32_t)length,dataCrc(data,length)};
  if(esp_partition_erase_range(part,0x10000,0x10000)!=ESP_OK||esp_partition_write(part,0x10000+sizeof(h),data,length)!=ESP_OK||esp_partition_write(part,0x10000,&h,sizeof(h))!=ESP_OK)return false;
  adminTransactionBlocked=true;return recoverAdminTransaction();
}
static void scheduleManagementRestart() {restartPending=true;restartAt=millis()+2500;}
static void handleBackup() {
  if(!sensitiveAccess("BACKUP"))return;
  String kind=server.arg("kind");uint32_t flags=kind=="products"?1:(kind=="settings"?2:(kind=="full"?3:0));
  if(!flags){sendMessage(400,false,"kind must be products, settings or full");return;}
  ensureProductsLoaded();size_t size=payloadLength(flags);auto data=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[sizeof(BackupHeader)+size]());
  if(!data){sendMessage(503,false,"Insufficient heap");return;}auto h=(BackupHeader*)data.get();
  *h={BACKUP_MAGIC,1,flags,(uint32_t)size,sizeof(Config),sizeof(DeviceSettings),sizeof(products),0};size_t offset=sizeof(*h);
  if(flags&BK_SETTINGS){CameraSnapshot snapshot={};if(!cameraReadSnapshot(snapshot)){sendMessage(503,false,"No telemetry");return;}BackupSettings image={};image.climate=snapshot.config;image.network=device;memcpy(data.get()+offset,&image,sizeof(image));offset+=sizeof(image);}
  if(flags&BK_PRODUCTS)memcpy(data.get()+offset,products,sizeof(products));
  h->crc=dataCrc(data.get()+sizeof(*h),size);noCache();server.sendHeader("Content-Disposition","attachment; filename=ClimateCamera_1.4.1_"+kind+".ccbackup");
  server.setContentLength(sizeof(*h)+size);server.send(200,"application/octet-stream","");
  size_t sent=0,total=sizeof(*h)+size;uint32_t last=millis();
  while(sent<total&&server.client().connected()&&(uint32_t)(millis()-last)<5000){size_t n=server.client().write(data.get()+sent,std::min((size_t)1024,total-sent));if(n){sent+=n;last=millis();}else vTaskDelay(1);}
}
static std::unique_ptr<uint8_t[]> restoreData;static size_t restoreLength=0;static bool restoreAllowed=false,restoreFailed=false;
static constexpr size_t MAX_BACKUP_SIZE=sizeof(BackupHeader)+sizeof(BackupSettings)+sizeof(products);
static void handleRestoreUpload() {
  HTTPUpload &up=server.upload();
  if(up.status==UPLOAD_FILE_START){restoreData.reset();restoreLength=0;restoreFailed=false;restoreAllowed=sensitiveAccess("RESTORE");if(restoreAllowed){restoreData.reset(new(std::nothrow) uint8_t[MAX_BACKUP_SIZE]);if(!restoreData)restoreFailed=true;}}
  else if(up.status==UPLOAD_FILE_WRITE&&restoreAllowed&&!restoreFailed){if(up.currentSize>MAX_BACKUP_SIZE-restoreLength){restoreFailed=true;return;}memcpy(restoreData.get()+restoreLength,up.buf,up.currentSize);restoreLength+=up.currentSize;}
  else if(up.status==UPLOAD_FILE_ABORTED){restoreFailed=true;restoreData.reset();}
}
static void finishRestore() {
  if(!restoreAllowed)return;restoreAllowed=false;
  if(!authorized(true)){restoreData.reset();return;}
  BackupHeader h={};if(restoreFailed||restoreLength<sizeof(h)){restoreData.reset();sendMessage(400,false,"Upload failed or file too small");return;}
  memcpy(&h,restoreData.get(),sizeof(h));const uint8_t *image=restoreData.get()+sizeof(h);
  bool valid=h.magic==BACKUP_MAGIC&&h.format==1&&h.configSize==sizeof(Config)&&h.deviceSize==sizeof(DeviceSettings)&&h.productsSize==sizeof(products)&&h.length==restoreLength-sizeof(h)&&h.crc==dataCrc(image,h.length)&&payloadValid(h.flags,image,h.length);
  if(!valid){restoreData.reset();sendMessage(400,false,"Incompatible, corrupt or invalid backup; nothing changed");return;}
  bool ok=commitTransaction(h.flags,image,h.length);restoreData.reset();
  if(!ok){sendMessage(507,false,"Restore incomplete; system off; transaction retained for reboot recovery");return;}
  sendMessage(200,true,"Restored; rebooting. System remains OFF. Credentials/IP may change.");scheduleManagementRestart();
}
static void handleReset() {
  bool full=server.arg("kind")=="full";if(server.arg("kind")!="settings"&&!full){sendMessage(400,false,"Invalid reset type");return;}
  if(!sensitiveAccess(full?"RESET_ALL":"RESET_SETTINGS"))return;ensureProductsLoaded();
  if(!saveProductsImage(products)){sendMessage(507,false,"Products could not be preserved; reset cancelled");return;}
  uint32_t flags=BK_SETTINGS|(full?BK_PRODUCTS:0);size_t length=payloadLength(flags);
  auto data=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[length]());if(!data){sendMessage(503,false,"Insufficient heap");return;}
  BackupSettings settings={};setDefaultSettings(settings.climate);settings.climate.enabled=false;deviceDefaults(settings.network,true);memcpy(data.get(),&settings,sizeof(settings));
  if(!commitTransaction(flags,data.get(),length)){sendMessage(507,false,"Reset incomplete; system off; retry recovery after reboot");return;}
  // Legacy data must not reappear if the product store is subsequently reinitialised.
  if(full){Preferences p;if(p.begin("cc_prod",false)){p.clear();p.end();}}
  sendMessage(200,true,full?"Factory reset including products; rebooting":"Factory settings restored; products preserved; rebooting");scheduleManagementRestart();
}
static void handleDeviceInfo() {
  AuthRole role=AuthRole::None;if(!authorized(false,&role))return;if(role!=AuthRole::Admin){sendMessage(403,false,"Admin only");return;}
  String out="{\"ok\":true,\"ssid\":";appendJsonString(out,device.ssid);out+=",\"adminUser\":";appendJsonString(out,device.adminUser);out+=",\"viewerUser\":";appendJsonString(out,device.viewerUser);
  out+=",\"apSsid\":";appendJsonString(out,device.apSsid);out+=",\"ip\":";appendJsonString(out,device.ip);out+=",\"gateway\":";appendJsonString(out,device.gateway);out+=",\"subnet\":";appendJsonString(out,device.subnet);out+=",\"dns\":";appendJsonString(out,device.dns);
  out+=",\"staticIp\":";out+=device.staticIp?"true":"false";out+=",\"apFallback\":";out+=device.apFallback?"true":"false";
  out+=",\"needsSetup\":";out+=(strlen(device.ssid)==0||WiFi.status()!=WL_CONNECTED)?"true":"false";out+=",\"storageReady\":";out+=managementPartition()?"true":"false";out+="}";noCache();server.send(200,"application/json; charset=utf-8",out);
}
static void handleDeviceSave() {
  if(!sensitiveAccess("SAVE_DEVICE"))return;DeviceSettings next=device;bool ok=true;
  auto field=[&](const char *key,char *out,size_t cap,bool keepEmpty=false){if(!server.hasArg(key))return;String value=server.arg(key);if(keepEmpty&&value.length()==0)return;if(value.length()>=cap||value.indexOf('\0')>=0){ok=false;return;}value.toCharArray(out,cap);};
  field("ssid",next.ssid,sizeof(next.ssid));field("wifiPass",next.wifiPass,sizeof(next.wifiPass),true);
  field("adminUser",next.adminUser,sizeof(next.adminUser));field("adminPass",next.adminPass,sizeof(next.adminPass),true);
  field("viewerUser",next.viewerUser,sizeof(next.viewerUser));field("viewerPass",next.viewerPass,sizeof(next.viewerPass),true);
  field("apSsid",next.apSsid,sizeof(next.apSsid));field("apPass",next.apPass,sizeof(next.apPass),true);
  field("ip",next.ip,sizeof(next.ip));field("gateway",next.gateway,sizeof(next.gateway));field("subnet",next.subnet,sizeof(next.subnet));field("dns",next.dns,sizeof(next.dns));
  if(server.arg("clearWifiPass")=="1")next.wifiPass[0]=0;
  for(const char *key:{"staticIp","apFallback"})if(server.hasArg(key)&&server.arg(key)!="0"&&server.arg(key)!="1")ok=false;
  if(server.hasArg("staticIp"))next.staticIp=server.arg("staticIp")=="1";
  if(server.hasArg("apFallback"))next.apFallback=server.arg("apFallback")=="1";
  if(!ok||!deviceValid(next)){sendMessage(400,false,"Invalid fields: usernames 3-32 ASCII, passwords 8-63, distinct users, valid IPv4 network");return;}
  BackupSettings image={};CameraSnapshot snapshot={};if(!cameraReadSnapshot(snapshot)){sendMessage(503,false,"No telemetry");return;}image.climate=snapshot.config;image.climate.enabled=false;image.network=next;
  if(!commitTransaction(BK_SETTINGS,(const uint8_t*)&image,sizeof(image))){sendMessage(507,false,"Save incomplete; system off; reboot for recovery");return;}
  sendMessage(200,true,"Saved; rebooting. Reconnect using new network/credentials. System remains OFF.");scheduleManagementRestart();
}
static void handleWifiScan() {
  if(!authorized(true))return;
  if(WiFi.getMode()==WIFI_STA){}else if(WiFi.getMode()==WIFI_AP)WiFi.mode(WIFI_AP_STA);
  int count=WiFi.scanComplete();if(count==WIFI_SCAN_RUNNING){sendMessage(202,true,"Scanning");return;}
  if(count==WIFI_SCAN_FAILED){WiFi.scanNetworks(true,true);sendMessage(202,true,"Scanning");return;}
  String out="{\"ok\":true,\"networks\":[";bool first=true;for(int i=0;i<count&&i<40;i++){if(!first)out+=',';first=false;out+="{\"ssid\":";appendJsonString(out,WiFi.SSID(i).c_str());out+=",\"rssi\":"+String(WiFi.RSSI(i))+",\"secure\":";out+=WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"false":"true";out+='}';}
  out+="]}";WiFi.scanDelete();noCache();server.send(200,"application/json; charset=utf-8",out);
}
#include "AdminUI.h"
static void setupAdminRoutes() {
  for(const char *path:{"/admin","/setup"})server.on(path,HTTP_GET,[](){AuthRole role=AuthRole::None;if(!authorized(false,&role))return;if(role!=AuthRole::Admin){sendMessage(403,false,"Administrator required");return;}noCache();server.send_P(200,"text/html; charset=utf-8",ADMIN_HTML);});
  server.on("/api/admin/device",HTTP_GET,handleDeviceInfo);server.on("/api/admin/device",HTTP_POST,handleDeviceSave);
  server.on("/api/admin/wifi/scan",HTTP_POST,handleWifiScan);server.on("/api/admin/backup",HTTP_POST,handleBackup);
  server.on("/api/admin/restore",HTTP_POST,finishRestore,handleRestoreUpload);server.on("/api/admin/reset",HTTP_POST,handleReset);
}
