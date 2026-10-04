# XIAO ePaper Panel Driver Board

Arduino drivers for Seeed Studio XIAO boards to control various e-paper displays. Works seamlessly with [epaperpix.app](https://epaperpix.app) for easy content management and wireless updates.

## Overview

This project provides comprehensive Arduino drivers for interfacing Seeed Studio XIAO ESP32 boards with a wide range of e-paper displays. It includes both WiFi-enabled and serial communication options, making it versatile for various applications from digital signage to IoT displays.

## Features

- 🌐 **WiFi Connectivity**: Automatic content updates from epaperpix.app
- 🔌 **Serial Interface**: Direct control for standalone applications  
- 📱 **Multiple Display Support**: Compatible with 1.54" to 7.5" e-paper displays
- 🎨 **Color Support**: Monochrome, tri-color, and multi-color displays
- 💤 **Low Power**: Deep sleep support for battery-powered applications
- 🔧 **Easy Configuration**: Simple pin mapping and display selection

## Hardware Compatibility

### Supported XIAO Boards
- [Seeed Studio XIAO ESP32 Series](https://www.seeedstudio.com/xiao-series-page) (ESP32-C3, ESP32-S3, etc.)

### Compatible Hardware
- [XIAO 7.5" ePaper Panel](https://www.seeedstudio.com/XIAO-7-5-ePaper-Panel-p-6416.html) - Complete 7.5" e-paper solution
- [ePaper Driver Board for XIAO V2](https://www.seeedstudio.com/ePaper-breakout-Board-for-XIAO-V2-p-6374.html) - Latest driver board
- [ePaper Breakout Board for XIAO](https://www.seeedstudio.com/ePaper-Breakout-Board-p-5804.html) - Original breakout board

> **Note**: Screens from GoodDisplay and Seeed Studio are in most cases compatible.

## Display Type Indicators

When selecting a display driver, note the suffix letters indicate color capabilities:

- **A** = Black and White
- **B** = Black, White, and Red
- **E** = 6 colors (Spectra)
- **F** = 7 colors
- **G** = 4 colors (Black, White, Red, and Yellow)

## Project Structure

```
XIAOePaperPanelDriverBoard/
├── Arduino/
│   ├── epd_epaperpix_wifi/        # WiFi-enabled client for epaperpix.app
│   │   ├── epd_epaperpix_wifi.ino # Main WiFi sketch
│   │   ├── display_config.h       # Panel and carrier board selection
│   │   ├── epdif.h/cpp            # Hardware interface (pin map)
│   │   ├── epd_base.h             # Base display class
│   │   └── epd*.cpp               # Individual display drivers
│   └── epd_serial/                # Serial interface for direct control
│       ├── epd_serial.ino         # Main serial sketch
│       ├── display_config.h       # Panel and carrier board selection
│       ├── epdif.h/cpp            # Hardware interface (pin map)
│       ├── epd_base.h             # Base display class
│       └── epd*.cpp               # Individual display drivers
├── LICENSE                        # MIT License
└── README.md                      # This file
```

## Getting Started

### 1. Select Your Carrier Board

Both sketches read `display_config.h`. The two SeeedStudio boards differ only in the BUSY pin:

```cpp
#define EPD_XIAO_DRIVER      // ePaper Driver Board / XIAO 7.5" ePaper Panel: BUSY = D2 (default)
// #define EPD_XIAO_BREAKOUT // ePaper Breakout Board: BUSY = D5
```

RST = D0, DC = D3, CS = D1, SCK = D8 and MOSI = D10 on both. The sketch prints the selected board at boot.

### 2. Select Your Display

In `display_config.h`, set the default panel define (for example `#define EPD2IN9_C`), or pass `-DEPD2IN9_C` to the compiler. Exactly one panel may be selected.


### 3. Upload and Run

1. Open the sketch in Arduino IDE
2. Select your XIAO board type
3. Upload the sketch
4. For WiFi version: Connect to AP "epaperpix_WiFi_Setup" to configure

## Usage

### WiFi Mode (epd_epaperpix_wifi)

1. Power on the device
2. Connect to WiFi AP: `epaperpix_WiFi_Setup` (password `configme`)
3. Open browser to `192.168.4.1`
4. Enter your WiFi credentials and epaperpix.app device code
5. Device will automatically fetch and display content

The portal reopens by itself after three wakes without WiFi or three wakes where the server rejects the device code. Blank fields keep the saved values; the saved device code is never shown (only its last four characters). Reset (tick the box, then press Reset) erases the WiFi settings and the device code.

### Serial Mode (epd_serial)

1. Connect via serial at 115200 baud and wait for `Ready`
2. Send one frame: `EPX1`, the payload length (uint32 little-endian), the payload (the cloud `.bin`), and its CRC-32 (`zlib.crc32`, uint32 little-endian). Headerless raw frames are still accepted. Do not send a `0x24` byte first.
3. Useful for debugging or standalone applications

## Supported Displays

| Display | Size | Colors | Type | Define |
|---------|------|---------|------|---------|
| EPD1IN54 | 1.54" | B/W | A | `EPD1IN54_C` |
| EPD1IN54B | 1.54" | B/W/Red | B | `EPD1IN54B_C` |
| EPD2IN13_V2/V3 | 2.13" | B/W | A | `EPD2IN13_V2_C` / `EPD2IN13_V3_C` |
| EPD2IN13G | 2.13" | 4 Color | G | `EPD2IN13G_C` |
| EPD2IN66G | 2.66" | 4 Color | G | `EPD2IN66G_C` |
| EPD2IN7 | 2.7" | B/W | A | `EPD2IN7_C` |
| EPD2IN7B | 2.7" | B/W/Red | B | `EPD2IN7B_C` |
| EPD2IN9 | 2.9" | B/W | A | `EPD2IN9_C` |
| EPD3IN97G | 3.97" | 4 Color | G | `EPD3IN97G_C` |
| EPD4IN01F | 4.01" | 7 Color | F | `EPD4IN01F_C` |
| EPD5IN79 | 5.79" | B/W | A | `EPD5IN79_C` |
| EPD7IN3F | 7.3" | 7 Color | F | `EPD7IN3F_C` |
| EPD7IN3G | 7.3" | 4 Color | G | `EPD7IN3G_C` |
| EPD7IN5 (V1) | 7.5" | B/W | A | `EPD7IN5_C` |
| EPD7IN5_V2 | 7.5" | B/W | A | `EPD7IN5_V2_C` |
| EPD7IN5B_V2 | 7.5" | B/W/Red | B | `EPD7IN5B_C` |

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Based on Waveshare e-paper library
- Modified and enhanced for Seeed Studio XIAO boards
- Developed by [EpaperPix](https://epaperpix.app)

## Support

For issues, feature requests, or contributions, please visit our [GitHub repository](https://github.com/yourusername/XIAOePaperPanelDriverBoard).