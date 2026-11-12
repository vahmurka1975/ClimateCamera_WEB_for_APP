# Проверка комплекта v1.4.5

Сборка PlatformIO (`esp32dev`): **SUCCESS** (`build_v145_final.log`).
- **RAM:** 85 060 / 327 680 байт (26,0%)
- **Flash:** 1 368 981 / 1 966 080 байт (69,6%)
- **Размер `firmware.bin`:** 1375280 байт (`SHA-256: 2ada641ea7161254d738bc8f3b764a3c2e747875721794b31a87068b7c87940c`)

Проверка `esptool.py image_info` (`image_info_v145.log`): контрольная сумма (`Checksum: a4 (valid)`) и `Validation Hash` (`d6977074ed50bf1b1745a52b3f1a013263d4745d8d64512f117bdcad849ae9ea (valid)`), 5 сегментов, точка входа `0x4008332c`.

Таблица разделов `partitions_140.csv`:
- `nvs`: `0x9000`, `0x5000` (20 КБ)
- `otadata`: `0xe000`, `0x2000` (8 КБ)
- `app0`: `0x10000`, `0x1e0000` (1920 КБ)
- `app1`: `0x1f0000`, `0x1e0000` (1920 КБ)
- `cc_store`: `0x3d0000`, `0x30000` (192 КБ)

Ключевые изменения в v1.4.5:
1. Исправлена ошибка ложного срабатывания проверки нулевого байта в `handleDeviceSave()` (`AdminOperations.inc`): вместо `value.indexOf('\0') >= 0` используется `memchr(value.c_str(), '\0', value.length()) != nullptr`.
2. Сохранена подробная русскоязычная валидация сетевых параметров и учётных записей в `deviceValidationError()` (`DeviceSettings.inc`).
3. Сквозное обновление версии до `1.4.5` (`ClimateCamera_1_4_5_IDE1819`).
