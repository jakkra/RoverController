# Rover Controller

## How it works
Sets up an AP, the Rover will automatically connect if it's in range.
The Rover is controlled by sending joystick and switch state over either LoRa or a websocket. If the Rover isn't connected to the AP, then data will be sent over LoRa instead. There is a switch to override this behaviour and always send data ove LoRa even if th Rover is connected to the AP.

When the rover is connected to the local AP it will send telematics data over the websocket, which then in turn is passed on to a phone if one is connected. This bridging is necessary as I plan to also send the telematics from the Rover using LoRa in the future to allow telematics when Rover is far away.

A phone can connect to the Controller AP to view the telematics from the Rover. Phone opens a websocket connection to the Controller and receives the telematic data the Rover sends. Telematics website can be found here: https://github.com/jakkra/Rover-Mission-Control.

## CAD model
To be uploaded when finished

## Hardware
- [ESP32 + LoRa module](https://www.banggood.com/2Pcs-LILYGO-TTGO-LORA32-868Mhz-ESP32-LoRa-OLED-0_96-Inch-Blue-Display-bluetooth-WIFI-ESP-32-Development-Board-Module-With-Antenna-p-1507044.html?rmmds=myorder&cur_warehouse=CN)
- 2x [Joysticks](https://www.ebay.com/itm/4-Axis-Joystick-Potentiometer-Button-For-JH-D400X-R4-10K-4D-with-Wire/313002251456?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649)
- 1x ADS1115 (ESP32 does not have enough ADCs for all the pots)
- A few on-off-on switches
- 2 potentiometers
- 2 colored LEDs

## Config
For pinmapping see: https://github.com/jakkra/RoverController/blob/master/main/controller_input.c#L46

## Images
<img src="/.github/render.png "/>


