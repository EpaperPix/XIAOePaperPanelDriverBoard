#pragma once
// Select exactly one panel here, or define it in compiler flags.
#if (defined(EPD1IN54B_C) + defined(EPD1IN54B_V2_C) + defined(EPD1IN54_C) + defined(EPD1IN54_V2_C) + defined(EPD2IN13G_C) + defined(EPD2IN13_V2_C) + defined(EPD2IN13_V3_C) + defined(EPD2IN66G_C) + defined(EPD2IN7B_C) + defined(EPD2IN7_C) + defined(EPD2IN9_C) + defined(EPD3IN97G_C) + defined(EPD4IN01F_C) + defined(EPD5IN79_C) + defined(EPD7IN3F_C) + defined(EPD7IN3G_C) + defined(EPD7IN5B_C) + defined(EPD7IN5_C) + defined(EPD7IN5_V2_C)) == 0
#define EPD2IN9_C
#endif
#if (defined(EPD1IN54B_C) + defined(EPD1IN54B_V2_C) + defined(EPD1IN54_C) + defined(EPD1IN54_V2_C) + defined(EPD2IN13G_C) + defined(EPD2IN13_V2_C) + defined(EPD2IN13_V3_C) + defined(EPD2IN66G_C) + defined(EPD2IN7B_C) + defined(EPD2IN7_C) + defined(EPD2IN9_C) + defined(EPD3IN97G_C) + defined(EPD4IN01F_C) + defined(EPD5IN79_C) + defined(EPD7IN3F_C) + defined(EPD7IN3G_C) + defined(EPD7IN5B_C) + defined(EPD7IN5_C) + defined(EPD7IN5_V2_C)) != 1
#error "Select exactly one e-paper display"
#endif

// Carrier board for the XIAO targets. The two SeeedStudio boards differ only in the BUSY pin:
//   EPD_XIAO_BREAKOUT  ePaper Breakout Board for XIAO           BUSY = D5
//   EPD_XIAO_DRIVER    ePaper Driver Board for XIAO / XIAO 7.5" ePaper Panel   BUSY = D2
// Define one here or in compiler flags. A wrong choice no longer hangs the device (BUSY waits time out),
// but the panel will not refresh correctly.
#if !defined(EPD_XIAO_BREAKOUT) && !defined(EPD_XIAO_DRIVER)
#define EPD_XIAO_DRIVER
#endif

// Fast refresh on timer wakes: only the changed pixels redraw, with no full-screen flash. Set true or false.
// Supported panels: EPD2IN9_C; other panels ignore it. A cold boot and the setup QR still do a full
// refresh, and so does the draw after every EPD_FULL_REFRESH_EVERY fast ones (default 60, see epd_base.h).
#ifndef EPD_FAST_REFRESH
#define EPD_FAST_REFRESH true
#endif

// Verbose serial log for debugging: API replies and timings, download sizes and times, refresh time, RTC state
// at boot and time awake per wake. Set true or false. Device codes and SAS tokens are never printed.
#ifndef EPD_VERBOSE
#define EPD_VERBOSE true
#endif
