# Arduino weather station with telegram alerting system

Basic weather station (temperature, humidity and pressure) with a simple web server and telegram bot to query and alert based on boundaries.
 
## Requirements

 * Arduino board (this code was tested on WEMOS D1 R1 and WEMOS D1 mini) 
 * [DHT 22](https://www.adafruit.com/product/385)
 * [BMP180](https://www.adafruit.com/product/1603)

## Prerequisites

Flash your Arduino and prepare your Arduino IDE with the following libraries:

 - [DHT](https://github.com/adafruit/DHT-sensor-library)
 - [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
 - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
 - [UniversalArduinoTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/)
 - [SFE_BMP180](https://github.com/jcastillocano/SFE_BMP180)

### Notes
With latest TelegramBot versions, we don't need to downgrade ESP8266 board neither ArduinoJSON library versions. If you use an older version, please see notes below:

Note: for my WEMOS boards, I had to install ESP8266 board version 2.5.2 instead of latest 2.6.3, otherwise Telegram Bot won't work.
![ESP8266 Board version](images/arduinojson-version.png?raw=true "ESP8266 Board version")
Note: another issue was JSON buffer errors, to avoid them I had to downgrade ArduinoJSON library to 5.13.5 (latest version 6.15.1 at this time).
![ArduinoJSON version](images/esp8266-board-version.png?raw=true "ArduinoJSON version")


## Pinout

DHT 22 sensor just need three pins:
 
 * First leg: power (3.3V or 5V)
 * Second leg: connect it to any I/O port in your board. Change DHTPIN constant in the code.
 * Third leg: not used
 * Four leg: ground

BMP 180 needs 4 pins:
 * First leg (SDA): D1
 * Second leg (SCL): D2
 * Third leg (GND): ground
 * Fourth leg (VIN): power (3.3V or 5V)

See https://cdn-learn.adafruit.com/downloads/pdf/dht.pdf and for more references.

![Sensor pinout](images/sensor.jpeg?raw=true "Sensor pinout")

## Installation

Replace all secrets with your custom values:

 * **ssid/password** for you wifi connection
 * **telegram_token/chatId** for your telegram bot config

Update max/min temperature and humidity levels to fit your requirements. Current values are:

```
const float maxTemp = 28.0; // Max temperature for alerting system
const float minTemp = 15.0; // Min temperature for alerting system
const float maxHum = 85.0;  // Max humidity for alerting system
const float minHum = 50.0;  // Min humidity for alerting system
```

Finally upload _weatherstation.ino_ to your board.

## Debugging

Open your serial monitor (9600 baudios), wait for your private IP to open it on your browser (remember, same wifi network!), you should see a basic webpage with both temperature and humidity!

## Author

Juan Carlos Castillo Cano
