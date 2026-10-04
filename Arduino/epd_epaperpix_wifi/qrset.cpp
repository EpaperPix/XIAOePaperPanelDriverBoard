/**
 *  @filename   :   qrset.cpp
 *  @brief      :   QR code display functions
 *  
 *
 *  MIT License
 *  
 *  Copyright (c) 2025 EpaperPix
 * 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "epd_base.h"

#ifndef QRSET_C
#define QRSET_C

// What QRset writes into each plane, in the format of the driver's cloud stream (screenCode).
enum QrPlane { QR_DRAW, QR_ALL_LIGHT, QR_ALL_ZERO };

static QrPlane qrPlaneFor(const char* code, int step) {
    if (step == 0) return QR_DRAW;
    if (strcmp(code, "V2A") == 0) return QR_DRAW;          // two identical planes
    if (strcmp(code, "B") == 0 || strcmp(code, "BR") == 0) return QR_ALL_ZERO;   // red plane: no red
    return QR_ALL_LIGHT;                                    // AS: right-hand controller stays white
}

// Stream value of a white pixel for a 1-bit plane of this Builder code (FuncAppEpaperBuilder ImageHandler):
// A, V2A, BR plane 0 = black is 1; AR, AS, ARS, B plane 0 = white is 1.
static unsigned char qrWhiteBit(const char* code) {
    return (strcmp(code, "A") == 0 || strcmp(code, "V2A") == 0 || strcmp(code, "BR") == 0) ? 0 : 1;
}

// Draws the setup QR into every plane. The pixels are produced in the driver's cloud-stream format and
// sent through SendStreamByte, so the QR gets exactly the polarity and expansion a downloaded image gets.
// Rows use the RAM stride (blockSize / height) and the scale is reduced until the code fits the panel.
void Epd::QRset( int scale) {
    if(!PixelFormatValid()) {
        Serial.println("QRset: driver pixel format is not set; nothing drawn");
        return;
    }
    unsigned long rowBytes = blockSize / height;                         // e.g. 16 for the 122-px 2.13" V3
    unsigned long planeWidth = rowBytes * pixels_per_byte;               // e.g. 400 per 5.79" controller
    unsigned long visibleWidth = width < planeWidth ? width : planeWidth;
    unsigned long fit = (visibleWidth < height ? visibleWidth : height) / QRDIM;
    if(fit < 1) fit = 1;
    if(scale < 1 || (unsigned long)scale > fit) scale = (int)fit;
    unsigned char light, dark;
    if(bits_per_pixel == 1) { light = qrWhiteBit(screenCode); dark = light ^ 1; }
    else { light = 0x1; dark = 0x0; }                                    // G and F palettes: 0 black, 1 white

    if(ShowDebug) {
        Serial.print("QRset: code ");
        Serial.print(screenCode);
        Serial.print(", ");
        Serial.print(bits_per_pixel);
        Serial.print(" bpp, plane ");
        Serial.print(planeWidth);
        Serial.print("x");
        Serial.print(height);
        Serial.print(", scale ");
        Serial.print(scale);
        Serial.print(", steps ");
        Serial.println(steps);
    }

    if(bits_per_pixel == 4 && !expand_1bpp_to_4bpp) {
        // ACeP controllers: restate the resolution before the data transmission
        SendCommand(0x61);
        SendData(width >> 8);
        SendData(width & 0xff);
        SendData(height >> 8);
        SendData(height & 0xff);
    }

    for(int step = 0; step < steps; step++) {
        QrPlane kind = qrPlaneFor(screenCode, step);
        SendCommand(stepCommands[step]);
        SetToDataMode();
        for(unsigned long row = 0; row < height; row++) {
            unsigned long qr_row = row / scale;
            for(unsigned long col_byte = 0; col_byte < rowBytes; col_byte++) {
                unsigned char packed = 0;
                if(kind == QR_ALL_ZERO) {
                    packed = 0;
                } else {
                    for(int pixel = 0; pixel < pixels_per_byte; pixel++) {
                        unsigned long qr_col = (col_byte * pixels_per_byte + pixel) / scale;
                        bool isDark = kind == QR_DRAW && qr_row < QRDIM && qr_col < QRDIM
                                      && get_bit(wifi_qrcode_32x32_data, qr_row * QRDIM + qr_col);
                        packed = (unsigned char)((packed << bits_per_pixel) | (isDark ? dark : light));
                    }
                }
                SendStreamByte(packed, step);
            }
        }
    }

    if(ShowDebug) {
        Serial.print("QRset: drew a ");
        Serial.print(QRDIM * scale);
        Serial.println(" px QR code");
    }
}

uint8_t Epd::get_bit(const uint8_t *array, size_t bit_position) {
    size_t byte_index = bit_position / 8;
    size_t bit_index = bit_position % 8;
    return (array[byte_index] >> (7 - bit_index)) & 1;
}

#endif
