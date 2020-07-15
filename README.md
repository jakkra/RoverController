# Rover Controller
Controller for driving [https://github.com/jakkra/Mars-Rover](https://github.com/jakkra/Mars-Rover).
Webpage for viewing telematics graphs on phone [https://github.com/jakkra/Rover-Mission-Control](https://github.com/jakkra/Rover-Mission-Control).

## How it works
Sets up an AP, the Rover will automatically connect if it's in range.
The Rover is controlled by sending joystick and switch state over either LoRa or a websocket. If the Rover isn't connected to the AP, then data will be sent over LoRa instead. There is a switch to override this behaviour and always send data over LoRa even if the Rover is connected to the AP.

When the Rover is connected to the local AP it will send telematics data over the websocket, or over LoRa if the Rover is outside of WiFi range. The data is then in turn passed on to a phone if one is connected. When the Rover is close by telematics will arrive about 10/s over WiFi, if outide of range then telematics are transported over LoRa. As LoRa modules cannot do true duplex data transfer we need to switch between sending and receiving, meaning telematics sent by the rover while we are sending joystick data will be lost. From experimentation LoRa telematics arrive about every 500ms. 

A phone can connect to the Controller AP to view the telematics from the Rover. Phone opens a websocket connection to the Controller and receives the telematic data the Rover sends. Telematics website can be found here: https://github.com/jakkra/Rover-Mission-Control.

## CAD model
Full Fusion 360 project is found in `CAD` folder. 

## Hardware
- [ESP32 + LoRa module](https://www.banggood.com/2Pcs-LILYGO-TTGO-LORA32-868Mhz-ESP32-LoRa-OLED-0_96-Inch-Blue-Display-bluetooth-WIFI-ESP-32-Development-Board-Module-With-Antenna-p-1507044.html?rmmds=myorder&cur_warehouse=CN)
- 2x [Joysticks](https://www.ebay.com/sch/i.html?_from=R40&_trksid=m570.l1313&_nkw=JH-D400X-R4-10K-4D&_sacat=0)
- 1x ADS1115 (ESP32 does not have enough ADCs for all the pots)
- A few on-off-on switches
- 2 potentiometers
- 2 colored LEDs

## Config
For pinmapping see: https://github.com/jakkra/RoverController/blob/master/main/controller_input.c#L46

## Images
<img src="/.github/contoller_rover.jpg "/>
<img src="/.github/controller.jpg "/>
<img src="/.github/interface.jpg "/>
<img src="/.github/render.png "/>
<img src="/.github/full.jpg "/>

## Compiling
Follow instruction on [https://github.com/espressif/esp-idf](https://github.com/espressif/esp-idf) to set up the esp-idf, then just run `idf.py build` or use the [VSCode extension](https://github.com/espressif/vscode-esp-idf-extension).

## Building the controller
TBD upon request.

## esp-idf patches
Run `git apply esp-idf-patch/no_delay.patch` in your esp-idf folder. This decreases lag for TCP as it will flush the buffers after every write instead of LWIP buffering TCP data to send it in chunks.

