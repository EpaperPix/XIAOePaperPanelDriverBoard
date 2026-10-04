# 2.9inch e-Paper version detector

One Arduino sketch that tries every 2.9inch **black/white** driver in this package
(V3, V2, V1, D flexible, DES) and writes on the panel which driver produced the image.
The attempt that renders crisp, complete text tells you which panel you own.

Sketch: `EPD_2in9_Detector/EPD_2in9_Detector.ino`. Open it in the Arduino IDE, pick your
board, upload, open the Serial Monitor at 115200 baud, and watch the panel.

## Boards and wiring

The pin map is chosen automatically from the board you select in the IDE (`DEV_Config.h`).

### SeeedStudio XIAO on the ePaper Driver / ePaper Breakout board (primary target)

Select **XIAO_ESP32S3**, **XIAO_ESP32C3** or **XIAO_ESP32C6** under *Tools > Board > ESP32 Arduino*
(the XIAO entry, not a generic "Dev Module", so the `D0`..`D10` pin names exist).
The wiring is the one the `epd_epaperpix_wifi` and `epd_serial` sketches use in `epdif.h`:

| Signal | XIAO pin | ESP32-S3 GPIO | ESP32-C3 GPIO | ESP32-C6 GPIO |
|---|---|---|---|---|
| SCK  | D8 (`SCK`)   | 7  | 8  | 19 |
| DIN (MOSI) | D10 (`MOSI`) | 9 | 10 | 18 |
| CS   | D1 | 2 | 3 | 1 |
| DC   | D3 | 4 | 5 | 21 |
| RST  | D0 | 1 | 2 | 0 |
| BUSY | D2 on the **ePaper Driver** board, D5 on the **ePaper Breakout** board | 3 / 6 | 4 / 7 | 2 / 23 |

The two carrier boards differ only in BUSY. The default is the ePaper Driver board (BUSY=D2).
For the Breakout board uncomment `#define EPD_XIAO_BREAKOUT` near the top of `DEV_Config.h`.
There is no PWR pin on either board. The sketch prints the resolved GPIO numbers on the
Serial Monitor at start-up so you can check the map against your board.

If you see no Serial output on a XIAO, set *Tools > USB CDC On Boot: Enabled*. The sketch
waits up to 3 s for the port and then runs regardless, so the panel works either way.

### Other boards (fallback maps)

| Signal | Arduino UNO R4 | ESP32 (Waveshare) | ESP32-S3 (Waveshare) |
|---|---|---|---|
| SCK  | 13 | 13 | 12 |
| DIN (MOSI) | 11 | 14 | 11 |
| CS   | 10 | 15 | 10 |
| DC   | 9  | 27 | 13 |
| RST  | 8  | 26 | 14 |
| BUSY | 7  | 25 | 4  |
| PWR  | 6  | 33 | 5 (9-pin FPC only) |

Other boards: add a pin map block in `DEV_Config.h`; the sketch stops with a clear `#error`
until you do.

## What it does

1. Pulses RST and reads the BUSY pin. SSD168x-class panels (V1/V2/V3) idle LOW, UC8151-class
   panels (D/DES) idle HIGH. This only decides which group is tried first; both groups always run.
2. For each of the five drivers: init, draw a label, refresh, hold for 10 seconds, sleep.
   The label shows the attempt number, driver name, chip family, the BUSY verdict, the board,
   and a row of filled squares (one per attempt) that stays countable even if the text is garbled.
3. Prints a summary table to Serial with init time, refresh time and busy-timeout count per
   attempt, then stops. Press reset to run again.

Every busy wait is bounded (`DEV_WaitBusy`), so a driver aimed at the wrong chip fails within a
few seconds instead of hanging. A timeout count of 0 means the driver's handshake matched the
panel.

## Reading the result

- The attempt whose text is sharp and complete is your panel. Note its name and use the matching
  driver (`EPD_2in9_V3`, `EPD_2in9_V2`, `EPD_2in9`, `EPD_2in9d`) from the main package.
- V2 and V3 share the same controller. V2 loads its own waveform, V3 uses the factory one. Both
  inits usually produce an image on either panel; pick the one that looked cleaner, or check the
  version sticker on the flat cable.
- D and DES are also the same controller. D writes two RAM planes, DES one. Same advice.
- Attempts for the wrong chip family are harmless. They typically leave the panel unchanged or
  blank. The V1 driver on a V2/V3 panel (and vice versa) does refresh with a mismatched waveform,
  so that attempt can look faint or streaky rather than blank.
- If the BUSY probe reports the wrong family it only changes the order of the attempts.

## Files

- `DEV_Config.*`: unified pin map (XIAO ePaper Driver/Breakout first, then the UNO R4 and
  Waveshare ESP32 maps) and bit-banged SPI on every board, plus the bounded busy wait and
  probe helpers. Bit-banging means no dependency on which SPI peripheral a given XIAO
  (S3/C3/C6) exposes on D8/D10.
- `EPD_2in9*.{h,cpp}`: copies of the Arduino UNO R4 drivers with their `ReadBusy` bodies
  replaced by `DEV_WaitBusy`. `EPD_2in9_DES.*` is new, ported from the Raspberry Pi C driver,
  plus an explicit `0x61` resolution command (128×296) after `PSR 0x1F`, as `EPD_2in9d.cpp` does: without
  it a UC8151-class controller defaults to 96×230 and the DES attempt could look garbled on a real DES panel
  (not yet confirmed on hardware).
- `GUI_Paint.*`, `fonts.h`, `font12/16/20.cpp`: from the Arduino UNO R4 tree.
