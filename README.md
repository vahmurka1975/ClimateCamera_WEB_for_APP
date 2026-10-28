# ClimateCamera WEB for APP · 1.4.4

# ClimateCamera WEB for APP 1.4.4 — candidate build

Source base: https://github.com/vahmurka1975/ClimateCamera_WEB_for_APP (local downloaded main snapshot). Only the HTU21D/GY21 branch is used. No changes are pushed to GitHub.

Added /admin and /setup; persistent device profile in cc_device; editable Wi-Fi, DHCP/static IPv4, AP, admin/viewer accounts; asynchronous Wi-Fi scan. If no SSID, AP is immediate; failed station connection enables AP after 45 seconds. Existing Secrets.h remains the seed, NVS overrides it.

Three backup types (products/settings/full), matching restore types, CRC/format/size/range validation, current administrator password + CSRF + explicit confirmation. Passwords in backups are plaintext. Backups are firmware-format-specific, not universal JSON. Product-only restores preserve settings; settings-only restores preserve products.

Factory defaults with or without product deletion. Operations stop the climate controller, persist boot_off, and reboot with manual restart required. Restore/save/reset uses a replayable journal; product data uses two CRC-checked banks. Legacy product records are migrated and retained except after full reset. Logical deletion is not forensic flash erasure.

IMPORTANT: custom partition table required, supplied as partitions_140.csv. The original nvs/otadata/app0/app1 offsets are unchanged. Former SPIFFS region (unused by embedded Web UI) is now cc_store. First install via USB; a plain OTA update does NOT install the partition table. Back up existing products first. Never erase all flash when preserving NVS.

HTTP Basic Auth is not encrypted: trusted LAN or VPN only. Default AP: ClimateCamera-AP / Climate120!; admin: admin / ChangeMe_Admin_2026!; viewer: viewer / ChangeMe_View_2026!. Change all passwords on first access at http://192.168.4.1/setup.

Build target: esp32dev, espressif32 6.6.0 / Arduino-ESP32 2.0.14, TFT_eSPI 2.5.43. Hardware Wi-Fi/AP, power-loss recovery, relay safety and restore round trips require bench tests; compilation alone does not validate hardware behavior. See build/test reports in build/.

Installation: INSTALL_v1.4.4_RU.md

### Изменения в v1.4.4
- В обработчиках `handleProductWeigh()` и `handleProductRemove()` (`NetworkServer.cpp`) добавлена проверка результата записи во флеш-раздел `cc_store` (`saveProductSlotNvs(idx)`): при ошибке записи состояние слота в ОЗУ откатывается к предыдущему и клиенту возвращается HTTP `507 Flash write failed` (аналогично `handleProductAdd()`).
