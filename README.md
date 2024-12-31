# ESP32 BLE Control

This ESP32 firmware is mainly used to control the lighting of various modded brick models.
The main interface is a CLI over BLE but WiFi can also be enabled. A ReactJS based frontend is used for simple interaction
with the models.

## Setup

For a simple test setup connect a string of SK6812 RGB LEDs to pin 1 of an ESP32 dev module and upload the firmware
using `pio run -t upload` and the configuration using `pio run -t uploadfs`. A simple animation should be looped
automatically (from `data-template/bin/main.js`).

## Configuration

The firmware is designed so that on every model (using the same ESP32 board) has the same firmware version and model specific
configuration is purely done via files on a LittleFS filesystem.

### Colors

Colors can be given in various formats: `hsl`, `rgb`, `w` or names. A special color `primary` exists and is resolved to
the configured primary color in the LED configuration.

* `hsl(hue, saturation, lightness)`
* `rgb(red, green, blue)`
* `w(white_value)`
* `color_name(intensity)`

All values are in the range of 0 to 1. The `red`, `green`, `blue`, `white_value` and `intensity` values can exceed 1 if
the configured default brightness on the affected LEDs is less so that a value of 1 can always be configured as some kind
of default intensity and values greater than 1 can be used to generate some kind of flashing effects.

Named colors are configured in `/data/lib/colors.json`.

### LED Configuration

LEDs are configured in `/data/etc/leds.json`. See `data-template/etc/leds.json` for an example.

### Scripting

Complex animation sequences can be scripted using JavaScript. The library used is Jerryscript which supports asynchronous
execution to run multiple animation sequences in parallel. The main control interface is the same CLI as used for
the BLE control. A convenience library can be used from `/data/lib/led.js` for cleaner animation code.

Scripts are interpreted as ECMA modules.
