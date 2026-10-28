# Проверка комплекта v1.4.4

Сборка PlatformIO (`esp32dev`): **SUCCESS** (`build_v144_final.log`).
- **RAM:** 85 060 / 327 680 байт (26,0%)
- **Flash:** 1 368 977 / 1 966 080 байт (69,6%)
- **Размер `firmware.bin`:** 1375280 байт (`SHA-256: 6d96f37e2751874697bdf41c9e16753020160c69c1fe6c63f8b92f2e2b3123c5`)

Проверка `esptool.py image_info` (`image_info_v144.log`): контрольная сумма (`Checksum: a3 (valid)`) и `Validation Hash` (`86d3b0a3fc1c6c87adbcca7b82bbd088639c978afc074322fc36ede2a6609d70 (valid)`) корректны. Образ приложения помещается в раздел `app0` (`1 966 080` байт). Личный `Secrets.h` исключён из архива (оставлен шаблон `Secrets.example.h`).

## Ключевые изменения v1.4.4
1. **Детальная диагностика сохранения настроек сети и учётных записей (`DeviceSettings.inc` — `deviceValidationError`)**: вместо обезличенной ошибки `Invalid fields...` возвращается точное описание причины на русском языке (какое именно поле не прошло проверку: SSID, пароль Wi-Fi, логин/пароль администратора или наблюдателя, параметры резервной точки доступа или конфликт IP/шлюза/маски при статическом IPv4).
2. **Улучшенная обработка формы `/api/admin/device` (`AdminOperations.inc` — `handleDeviceSave`)**:
   - Автоматическая обрезка случайных пробелов по краям (`trim()`) для `ssid`, логинов, `apSsid` и IPv4-адресов (`ip`, `gateway`, `subnet`, `dns`), часто добавляемых автозаполнением на смартфонах.
   - Поддержка всех форматов булевых флагов (`1`/`0`, `true`/`false`, `on`/`off`) для `staticIp`, `apFallback` и `clearWifiPass`.
   - Сохранение прежних значений при пустых полях паролей и учётных записей (`keepEmpty = true`), поддержка необязательной учётной записи наблюдателя (`viewerUser`).
3. **Повышение версии прошивки до `1.4.4`** (каталог `ClimateCamera_1_4_4_IDE1819`, `FirmwareVersion.h`, `WebUI.h`, `AdminUI.h`, `AdminOperations.inc`, `ClimateCamera.cpp`, `User_Setup.h`, `platformio.ini`, `MANIFEST.json`, `INSTALL_v1.4.4_RU.md`, `RELEASE_NOTES_v1.4.4.md`).
