# Проверка комплекта v1.4.2

Сборка PlatformIO (`esp32dev`): **SUCCESS** (`build_v141_final.log`).
- **RAM:** 85 036 / 327 680 байт (26,0%)
- **Flash:** 1 362 413 / 1 966 080 байт (69,3%)
- **Размер `firmware.bin`:** 1368704 байт (`SHA-256: 012ba3876ecb4c16a1c9a2535f65fa2c963bb2ebba9445a6f41bec6b55b6e9aa`)

Проверка `esptool.py image_info` (`image_info_v141.log`): контрольная сумма (`Checksum: 43 (valid)`) и `Validation Hash` (`valid`) корректны. Образ приложения помещается в раздел `app0` (`1 966 080` байт). Личный `Secrets.h` исключён из архива (оставлен шаблон `Secrets.h.example`).

## Ключевые изменения v1.4.2
1. Контроль записи во флеш-память NVS (`saveProductSlotNvs`) в обработчиках `/api/products/weigh` и `/api/products/remove` с откатом состояния слота в ОЗУ (`slot = previous;`) и возвратом HTTP `507 ("Flash write failed")` при сбое записи.
2. Синхронизация сообщения об ошибке записи в `/api/products/add` (`HTTP 507 "Flash write failed"`).
3. Повышение версии прошивки до `1.4.2` (скетч `ClimateCamera_1_4_2_IDE1819`, `FirmwareVersion.h`, `WebUI.h`, `AdminUI.h`, `AdminOperations.inc`, `ClimateCamera.cpp`, `User_Setup.h`, `platformio.ini`, `README.md`, `INSTALL_v1.4.2_RU.md`, `RELEASE_NOTES_v1.4.2.md`).
