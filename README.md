# WEATHERSTACK API DEMO

This is a simple web-enabled weather-reporting application that demonstrates the combination of Infineon PSoC 6 MCU functionality like CapSense with AnyCloud Wi-Fi software libraries. The Weatherstack API service is used to get the latest weather conditions at cities around the world. It was used in the 5/21/2020 webinar [Flexible IoT Software Development with ModusToolbox® and the AnyCloud™ SDK"](https://www.cypress.com/iot-advantedge-webinars).

## Overview

The main() function creates two threads - capsense_task() and weather_task() - then launches the FreeRTOS OS. The CapSense task initializes the block and enters a scanning loop to look at button0. When the button is pressed it sends a task notification to the Weather thread containing a pointer to the name of a city. The Weather thread waits for the notification then constructs an HTTP request message to get the current weather in that city. The returned JSON is parsed using the markgsaunders/cJSON library and the conditions are reported via UART and on a TFT shield.

## Supported Kits

- CY8CPROTO-062-4343W Prototyping kit
- CY8CKIT-062S2-43012 Pioneer kit (used in the webinar)
-- CY8CKIT-028-TFT LCD shield (optional add-on for the Pioneer kit)

## Setup
1. Install [ModusToolbox™ software](https://www.cypress.com/products/modustoolbox-software-environment) v2.1
2. Create a weatherstack.com account and get a free access key (32-digit code)
3. Add the line "https://github.com/markgsaunders/ModusToolbox-Manifests/raw/master/super-manifest.xml" to your ~/.modustoolbox/manifest.loc file
4. Use the ModusToolbox Project Creator tool to create a new project
     1. Choose the BSP for your kit
    2. Choose the "Weatherstack API Demo" project template
5. Optionally export to your preferred IDE and follow the instructions
      - make eclipse
      - make vscode
      - make ewarm8
      - make uvision
6. If you have a TFT shield, enable software support with Segger emWin
    1. Use the ModusToolbox Library Manager tool to add required libraries
    -- emwin
    -- CY8CKIT-028-TFT
    2. Edit the Makefile file to un-comment two lines
    -- "DEFINES+=TFT_SUPPORTED"
    -- "COMPONENTS+=EMWIN_OSNTS"
7. Edit private_data.h to add your router name and password, plus your API key
8. Build and program the kit (IDE-specific)
    - make build
    - make program / make qprogram

## Operation

Simply connect a serial terminal emulator such as PuTTY (115200, 8, n, 1) to the KitProg (debug) USB port, wait for the device to connect to your Wi-Fi network and prompt you to press the CapSense button. Press the button to change the city and observe the reported weather data.

If you have the CY8CKIT-028-TFT shield and a CY8CKIT-062S2-43012 kit connect the shield to the Pioneer it and observe the serial terminal data being mirrored on the TFT screen.

