# sonoffSocket

<!---
[![start with why](https://img.shields.io/badge/start%20with-why%3F-brightgreen.svg?style=flat)](http://www.ted.com/talks/simon_sinek_how_great_leaders_inspire_action)
--->
[![GitHub release](https://img.shields.io/github/release/elbosso/sonoffSocket/all.svg?maxAge=1)](https://GitHub.com/elbosso/sonoffSocket/releases/)
[![GitHub tag](https://img.shields.io/github/tag/elbosso/sonoffSocket.svg)](https://GitHub.com/elbosso/sonoffSocket/tags/)
[![GitHub license](https://img.shields.io/github/license/elbosso/sonoffSocket.svg)](https://github.com/elbosso/sonoffSocket/blob/master/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/elbosso/sonoffSocket.svg)](https://GitHub.com/elbosso/sonoffSocket/issues/)
[![GitHub issues-closed](https://img.shields.io/github/issues-closed/elbosso/sonoffSocket.svg)](https://GitHub.com/elbosso/sonoffSocket/issues?q=is%3Aissue+is%3Aclosed)
[![contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](https://github.com/elbosso/sonoffSocket/issues)
[![GitHub contributors](https://img.shields.io/github/contributors/elbosso/sonoffSocket.svg)](https://GitHub.com/elbosso/sonoffSocket/graphs/contributors/)
[![Github All Releases](https://img.shields.io/github/downloads/elbosso/sonoffSocket/total.svg)](https://github.com/elbosso/sonoffSocket)

Switching Sonoff Basic and Sonoff S20 (Webserver, local button and MQTT).

## Getting started
This project ist split in two parts: sonoffSimple/sonoffSimple.ino is just a simple demo as described [here](https://ct.de/yxgs).

The more advanced software (sonoffSocket.ino) supports MQTT, toggle, status and is actively developed. Just copy sonoffSocket.ino into your Arduino-IDE folder. To use mqtt you have to install external library "PubSubClient". Change your SSID and WiFi-Key and activate MQTT if needed (choose a topic too).
Connect Sonoff Basic or Sonoff S20 to your FTDI-Adapter and flash it using the Arduino-IDE.

## How it works
The socket can be controlled by opening <ip of socket>/on (to activate) and <ip of socket>/off to deactivate. If MQTT is enabled, send 1 or 0 to your chosen topic (defined at the beginning of your code).
Get the current status with <ip of socket>/state and toggle  with <ip of socket>/toggle

## More information
This repository is part of article ["Bastelfreundlich"](https://ct.de/yxgs) from German computer magazine "c't".
