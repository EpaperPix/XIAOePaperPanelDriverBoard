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
#include "epd_base.h"

#ifndef QRSET_C
#define QRSET_C

void Epd::QRset( int scale) {
    // QRset using steps/stepCommands (Clear-then-QRset approach)
    if(ShowDebug) {
        Serial.print("QRset: Displaying QR code with color=0x");
          Serial.println(qr_color);
        Serial.print(", scale=");
        Serial.println(scale);
        Serial.print("Display size: ");
        Serial.print(width);
        Serial.print("x");
        Serial.println(height);
        Serial.print("QRset: ");
        Serial.print(bits_per_pixel);
        Serial.print(" bits per pixel, ");
        Serial.print(pixels_per_byte);
        Serial.println(" pixels per byte");
        Serial.print("Steps: ");
        Serial.println(steps);
    }
    
    // Display-specific pre-setup
    if(bits_per_pixel == 4) {
        // 4.01" color needs resolution setup before data transmission
        SendCommand(0x61);  // Set resolution
        SendData(0x02);     // Width high byte (640 = 0x0280)
        SendData(0x80);     // Width low byte
        SendData(0x01);     // Height high byte (400 = 0x0190)
        SendData(0x90);     // Height low byte
        
        if(ShowDebug) {
            Serial.println("QRset: Set resolution for 4-bit color display");
        }
    }
    
    // Use configured steps and stepCommands (Clear() already handled buffer prep)
   // for(int step = 0; step < steps; step++) {
        SendCommand(stepCommands[steps-1]);
        SetToDataMode();
        
        if(ShowDebug) {
            Serial.print("QRset: Step ");
            Serial.print(steps-1);
            Serial.print(" using command 0x");
            Serial.println(stepCommands[steps-1], HEX);
        }
        
        // Generate QR data based on pixel format
        if(bits_per_pixel == 4) {
            // 4-bit color (2 pixels per byte)
            for(int display_row = 0; display_row < height; display_row++) {
                int qr_row = display_row / scale;
                int qr_col = 0;
                int x_scale_counter = 0;
                
                for(int display_col_pair = 0; display_col_pair < width/pixels_per_byte; display_col_pair++) {
                    UBYTE pixel_pair = 0;
                    
                    // Process pixels for this byte
                    for(int pixel = 0; pixel < pixels_per_byte; pixel++) {
                        UBYTE pixel_color;
                        
                        // Check if we're outside the QR code area
                        if(qr_row >= QRDIM || qr_col >= QRDIM) {
                            pixel_color = 0x1;  // White background
                        } else {
                            // Get bit from QR code data
                            int bit_position = qr_row * QRDIM + qr_col;
                            if(bit_position >= QRDIM * QRDIM) {
                                pixel_color = 0x1;  // White if out of bounds
                            } else {
                                uint8_t bit_value = get_bit(wifi_qrcode_32x32_data, bit_position);
                                pixel_color = bit_value? qr_color : 0x1;  // QR color or white
                            }
                        }
                        
                        // Pack pixel into byte (first pixel in upper nibble, second in lower)
                        if(pixel == 0) {
                            pixel_pair = (pixel_color << 4);
                        } else {
                            pixel_pair |= pixel_color;
                        }
                        
                        // Advance to next QR column when we've processed enough pixels
                        x_scale_counter++;
                        if(x_scale_counter >= scale) {
                            x_scale_counter = 0;
                            qr_col++;
                        }
                    }
                    
                    SendData(pixel_pair);
                }
            }
            
        } else if(bits_per_pixel == 1) {
            // 1-bit monochrome (8 pixels per byte)
            for(int display_row = 0; display_row < height; display_row++) {
                int qr_row = display_row / scale;
                
                for(int byte_col = 0; byte_col < width/8; byte_col++) {
                    UBYTE packed_byte = 0;
                    
                    // Pack 8 pixels into this byte (MSB first)
                    for(int bit = 0; bit < 8; bit++) {
                        int display_col = byte_col * 8 + bit;
                        int qr_col = display_col / scale;
                        
                        // Check if we're outside the QR code area  
                        if(qr_row >= QRDIM || qr_col >= QRDIM) {
                            // White background - set bit to 1
                            packed_byte |= (1 << (7-bit));
                        } else {
                            // Get bit from QR code data
                            int bit_position = qr_row * QRDIM + qr_col;
                            if(bit_position >= QRDIM * QRDIM) {
                                // White if out of bounds - set bit to 1
                                packed_byte |= (1 << (7-bit));
                            } else {
                                uint8_t qr_bit = get_bit(wifi_qrcode_32x32_data, bit_position);
                                if(!qr_bit) {
                                    // White pixel - set bit to 1 (black pixels leave bit as 0)
                                    packed_byte |= (1 << (7-bit));
                                }
                                // Black pixels: bit remains 0 (default)
                            }
                        }
                    }
                    
                    SendData(packed_byte);
                }
            }
        }
    //}
    
    if(ShowDebug) {
        Serial.println("QRset: Pixel data transmission complete");
        Serial.print("QRset: Scaled QR code from 32x32 to ");
        Serial.print(32 * scale);
        Serial.print("x");
        Serial.print(32 * scale);
        Serial.println(" pixels");
    }
}

uint8_t Epd::get_bit(const uint8_t *array, size_t bit_position) {
    size_t byte_index = bit_position / 8;
    size_t bit_index = bit_position % 8;
    return (array[byte_index] >> (7 - bit_index)) & 1;
}

#endif
