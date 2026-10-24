# Pixel Box: лампа ESP32 + WS2812B 16x16

ESP32 DevKit (CH340), матриця WS2812B 16x16, дані на D13 (GPIO13).

Можливості:
- Режими: Колір, Веселка, Вогонь, Конфетті, Дихання, Серце, Матриця, Сніг, Пакман, Світанок (15 хв від ночі до сонця, далі тримає кадр), Захід (15 хв сонце сідає, наприкінці лампа плавно гасне й вимикається). Обидва після кожного вмикання починаються спочатку. Пакман покадрово повторює `loop()` з `PacChaseLogo.ino` (MakerWorld #1133312).
- Вебсторінка на самій лампі. Завантаження фото, GIF і відео з чітким масштабуванням pixel art. Збережені анімації. Малювання.
- Трансляція з камери телефону через HTTPS і WebSocket.
- Home Assistant через MQTT discovery.
- Оновлення прошивки по Wi-Fi.

## Файли

- `lamp.ino`: прошивка (ефекти, LittleFS-анімації, MQTT для Home Assistant, OTA, HTTPS + WebSocket для камери).
- `page.h`: веб-інтерфейс (керування, медіа, малювання, налаштування).
- `cam.h`: HTTPS-сторінка трансляції з камери телефону.
- `pacman.h`: спрайти й палітри для режиму «Пакман», без змін узяті з оригіналу.
- `secrets.example.h`: зразок для `secrets.h` (Wi-Fi за замовчуванням, пароль запасної точки доступу).
- `gen_cert.sh`: генерує `cert.h` із самопідписаним сертифікатом для HTTPS.
- `presets/`: оригінальний `PacChaseLogo.ino` і `make_pacman_h.py`, який генерує з нього `pacman.h`.
- `matrix_test/`: простий RGB-тест матриці на NeoPixel.

`secrets.h` і `cert.h` не потрапляють у git.

## Перша збірка

```sh
cp secrets.example.h secrets.h    # вписати свій Wi-Fi
./gen_cert.sh 192.168.50.64       # IP лампи
```

Бібліотеки: Adafruit NeoPixel, FastLED 3.10.3, ArduinoJson 7.4.2, PubSubClient 2.8.0. Ядро: arduino-esp32 3.3.7.

Перший раз прошивка заливається по USB, і тільки з `--board-options UploadSpeed=115200`.

## Збірка й оновлення (по IP)

```sh
CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
"$CLI" compile --fqbn esp32:esp32:esp32 --output-dir build .
curl -F "fw=@build/lamp.ino.bin" http://192.168.50.64/update
```

Діагностика: `http://<IP>/crash` показує причину останнього перезапуску (`esp_reset_reason`: 9 = просідання живлення) і підсумок coredump останнього падіння.

IP видає DHCP. Якщо адреса змінилась, знайти її можна командою `arp -a | grep lamp`.
