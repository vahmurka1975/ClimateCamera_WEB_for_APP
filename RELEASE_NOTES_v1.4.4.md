# ClimateCamera WEB for APP v1.4.4

## Изменения в v1.4.4

- **Точная диагностика параметров `/api/admin/device` (`DeviceSettings.inc`):** функция `deviceValidationError()` проверяет каждое поле отдельно и возвращает понятное сообщение на русском языке при ошибке в SSID, пароле Wi-Fi, логине/пароле администратора или наблюдателя, настройках точки доступа (AP) или статических параметрах IPv4 (проверка маски подсети, совпадения подсети IP и шлюза, запрет совпадения IP камеры с IP роутера).
- **Повышение надёжности сохранения сетевых настроек из Web UI и мобильного приложения (`AdminOperations.inc` — `handleDeviceSave`):**
  - Автоматическое удаление случайных начальных и конечных пробелов (`trim()`) в полях `ssid`, `adminUser`, `viewerUser`, `apSsid`, `ip`, `gateway`, `subnet`, `dns`.
  - Приём булевых флагов `staticIp`, `apFallback`, `clearWifiPass` в форматах `1`/`0`, `true`/`false`, `on`/`off`.
  - Корректное сохранение существующих паролей при отправке пустых полей пароля; учётная запись наблюдателя (`viewerUser`/`viewerPass`) может быть оставлена пустой, если гостевой доступ не требуется.
- **Сквозное обновление версии до `1.4.4`:** скетч переименован в `ClimateCamera_1_4_4_IDE1819`, обновлены `FirmwareVersion.h`, `WebUI.h`, `AdminUI.h`, `AdminOperations.inc`, `ClimateCamera.cpp`, `User_Setup.h`, `platformio.ini` и скомпилирован актуальный бинарный образ `ClimateCamera_WEB_for_APP_v1.4.4_firmware.bin` (`1375280` байт, `SHA-256: 6d96f37e2751874697bdf41c9e16753020160c69c1fe6c63f8b92f2e2b3123c5`).
