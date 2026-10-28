// Persistent device settings. Kept separate from climate and products.
#pragma once
#include <Preferences.h>
#include <stddef.h>
struct DeviceSettings {
  uint32_t schema;
  char ssid[33], wifiPass[65];
  char adminUser[33], adminPass[64], viewerUser[33], viewerPass[64];
  char apSsid[33], apPass[64];
  char ip[16], gateway[16], subnet[16], dns[16];
  uint8_t staticIp, apFallback;
  uint8_t reserved[2];
};
static DeviceSettings device={};
static uint32_t dataCrc(const void *ptr,size_t len) {
  uint32_t c=0xffffffff;const uint8_t *p=(const uint8_t*)ptr;
  while(len--) {c^=*p++;for(int b=0;b<8;b++)c=(c>>1)^((c&1)?0xedb88320:0);}
  return ~c;
}
template<size_t N> static void textCopy(char (&out)[N],const String &s) {s.toCharArray(out,N);}
static void deviceDefaults(DeviceSettings &v,bool factory=false) {
  memset(&v,0,sizeof(v)); v.schema=1;
  textCopy(v.ssid,factory?String(""):String(CC_WIFI_SSID));
  textCopy(v.wifiPass,factory?String(""):String(CC_WIFI_PASS));
  textCopy(v.adminUser,CC_ADMIN_USER);textCopy(v.adminPass,CC_ADMIN_PASS);
  textCopy(v.viewerUser,CC_VIEWER_USER);textCopy(v.viewerPass,CC_VIEWER_PASS);
  textCopy(v.apSsid,CC_AP_SSID);textCopy(v.apPass,CC_AP_PASS);
  textCopy(v.ip,IPAddress(CC_STATIC_IP).toString());textCopy(v.gateway,IPAddress(CC_STATIC_GATEWAY).toString());
  textCopy(v.subnet,IPAddress(CC_STATIC_SUBNET).toString());textCopy(v.dns,IPAddress(CC_STATIC_DNS).toString());
  v.staticIp=factory?0:CC_USE_STATIC_IP;v.apFallback=factory?1:CC_AP_FALLBACK_ONLY;
}
static bool boundedText(const char *s,size_t cap,size_t minLen=0) {
  size_t n=strnlen(s,cap);if(n>=cap||n<minLen)return false;
  for(size_t i=0;i<n;i++)if((uint8_t)s[i]<32||s[i]==127)return false;
  return true;
}
static bool boundedUtf8(const char *s,size_t cap,size_t minLen=0) {
  size_t n=strnlen(s,cap);if(n>=cap||n<minLen)return false;
  for(size_t i=0;i<n;i++)if((uint8_t)s[i]<32||s[i]==127)return false;
  return true;
}
static bool validUser(const char *s) {
  if(!boundedText(s,33,3))return false;
  for(;*s;s++)if(!isalnum((unsigned char)*s)&&*s!='_'&&*s!='-'&&*s!='.')return false;
  return true;
}
static const char* deviceValidationError(const DeviceSettings &v) {
  if(v.schema!=1||v.staticIp>1||v.apFallback>1)return "Некорректная схема настроек (staticIp/apFallback)";
  if(!boundedUtf8(v.ssid,sizeof(v.ssid)))return "Некорректный SSID домашней сети Wi-Fi (макс. 32 байта)";
  if(!boundedText(v.wifiPass,sizeof(v.wifiPass)))return "Пароль Wi-Fi содержит недопустимые управляющие символы";
  size_t n=strlen(v.wifiPass);
  if(n!=0&&(n<8||n>64))return "Пароль Wi-Fi должен содержать от 8 до 63 символов (или быть пустым для открытой сети)";
  if(n==64){for(size_t i=0;i<n;i++)if(!isxdigit((unsigned char)v.wifiPass[i]))return "Пароль Wi-Fi длиной 64 символа должен быть HEX-ключом";}
  if(!validUser(v.adminUser))return "Логин администратора: 3–32 символа (латиница, цифры, '.', '_', '-')";
  if(strlen(v.viewerUser)>0 && !validUser(v.viewerUser))return "Логин наблюдателя: 3–32 символа (латиница, цифры, '.', '_', '-')";
  if(strlen(v.viewerUser)>0 && !strcmp(v.adminUser,v.viewerUser))return "Логин администратора и логин наблюдателя должны различаться";
  if(!boundedText(v.adminPass,sizeof(v.adminPass),8))return "Пароль администратора должен содержать от 8 до 63 символов";
  size_t vp=strlen(v.viewerPass);
  if(vp!=0 && !boundedText(v.viewerPass,sizeof(v.viewerPass),8))return "Пароль наблюдателя должен содержать от 8 до 63 символов (или оставьте пустым)";
  if(!boundedUtf8(v.apSsid,sizeof(v.apSsid),1))return "Укажите имя резервной точки доступа (AP SSID, 1–32 символа)";
  if(!boundedText(v.apPass,sizeof(v.apPass),8))return "Пароль точки доступа (AP) должен содержать от 8 до 63 символов";
  IPAddress ip,gw,mask,dns;
  if(!boundedText(v.ip,16)||!boundedText(v.gateway,16)||!boundedText(v.subnet,16)||!boundedText(v.dns,16))return "IP-параметры содержат недопустимые символы";
  if(!ip.fromString(v.ip))return "Некорректный формат IP-адреса (пример: 192.168.0.150)";
  if(!gw.fromString(v.gateway))return "Некорректный формат шлюза (пример: 192.168.0.1)";
  if(!mask.fromString(v.subnet))return "Некорректный формат маски подсети (пример: 255.255.255.0)";
  if(!dns.fromString(v.dns))return "Некорректный формат DNS-сервера (пример: 192.168.0.1 или 8.8.8.8)";
  if(v.staticIp) {
    uint32_t i=0,g=0,m=0;for(int k=0;k<4;k++){i=(i<<8)|ip[k];g=(g<<8)|gw[k];m=(m<<8)|mask[k];}
    uint32_t inv=~m;
    if(!m||inv<3||(inv&(inv+1)))return "Некорректная маска подсети для статического IP (пример: 255.255.255.0)";
    if(!i||!g||ip[0]>=224||gw[0]>=224)return "IP и шлюз должны быть корректными одноадресными IPv4-адресами";
    if((i&m)!=(g&m))return "Статический IP и шлюз должны находиться в одной подсети (проверьте первые три числа IP и шлюза)";
    if((i&inv)==0||(i&inv)==inv||(g&inv)==0||(g&inv)==inv)return "IP или шлюз совпадает с адресом сети или широковещательным адресом";
    if(i==g)return "Статический IP контроллера не должен совпадать с IP-адресом шлюза (роутера)";
  }
  return nullptr;
}
static bool deviceValid(const DeviceSettings &v) {
  return deviceValidationError(v)==nullptr;
}
struct DeviceRecord {DeviceSettings settings;uint32_t crc;};
static bool saveDevice(const DeviceSettings &v) {
  if(!deviceValid(v))return false;
  DeviceRecord b={};b.settings=v;b.crc=dataCrc(&b.settings,sizeof(b.settings));
  Preferences p;if(!p.begin("cc_device",false))return false;
  bool ok=p.putBytes("profile",&b,sizeof(b))==sizeof(b);DeviceRecord check={};
  ok=ok&&p.getBytes("profile",&check,sizeof(check))==sizeof(check)&&!memcmp(&b,&check,sizeof(b));p.end();return ok;
}
static bool loadDevice() {
  deviceDefaults(device);Preferences p;if(!p.begin("cc_device",true))return true;
  DeviceRecord b={};size_t len=p.getBytesLength("profile");
  bool ok=len==sizeof(b)&&p.getBytes("profile",&b,sizeof(b))==sizeof(b);p.end();
  if(!len)return true;
  if(!ok||b.crc!=dataCrc(&b.settings,sizeof(b.settings))||!deviceValid(b.settings))return false;
  device=b.settings;return true;
}
