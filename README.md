# dyack-home-garden
Arduino code to support an irrigation system.

# Dry Box Controller
A device used to regulate environmental conditions.

## Main Features
- Monitors temperature and humidity.
- Configurable using infrared remote control.
- Schedule adjustable at run time.
- Will retain current time and schedule in the event of power loss.

## Hardware Requirements

### Mainboard
Designed for use with an Arduino Mega 2560 Rev3.

### Modules
- DS3231 Real Time Clock
- HX-M121 Infrared Receiver
- DHT22 Temperature/Humidity Sensor
- LCD1602 Liquid Crystal Display

### External
- 12V power supply

## Library Requirements
All third party software requirements are available via the Arduino software Library Manager.

- [LinkedList](https://github.com/ivanseidel/LinkedList) v1.3.2
- [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) v3.5.1
- [DS3231](https://github.com/NorthernWidget/DS3231) v3.5.1
- [DHTlib](https://github.com/RobTillaart/DHTlib) v0.1.13
