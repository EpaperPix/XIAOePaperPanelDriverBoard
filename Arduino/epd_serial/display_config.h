#pragma once
// Select exactly one panel here, or define it in compiler flags.
#if (defined(EPD1IN54B_C) + defined(EPD1IN54B_V2_C) + defined(EPD1IN54_C) + defined(EPD1IN54_V2_C) + defined(EPD2IN13_V2_C) + defined(EPD2IN13_V3_C) + defined(EPD2IN66G_C) + defined(EPD2IN7B_C) + defined(EPD2IN7_C) + defined(EPD2IN9_C) + defined(EPD3IN97G_C) + defined(EPD4IN01F_C) + defined(EPD5IN79_C) + defined(EPD7IN3F_C) + defined(EPD7IN3G_C) + defined(EPD7IN5B_C) + defined(EPD7IN5_C) + defined(EPD7IN5_V2_C)) == 0
#define EPD4IN01F_C
#endif
#if (defined(EPD1IN54B_C) + defined(EPD1IN54B_V2_C) + defined(EPD1IN54_C) + defined(EPD1IN54_V2_C) + defined(EPD2IN13_V2_C) + defined(EPD2IN13_V3_C) + defined(EPD2IN66G_C) + defined(EPD2IN7B_C) + defined(EPD2IN7_C) + defined(EPD2IN9_C) + defined(EPD3IN97G_C) + defined(EPD4IN01F_C) + defined(EPD5IN79_C) + defined(EPD7IN3F_C) + defined(EPD7IN3G_C) + defined(EPD7IN5B_C) + defined(EPD7IN5_C) + defined(EPD7IN5_V2_C)) != 1
#error "Select exactly one e-paper display"
#endif
