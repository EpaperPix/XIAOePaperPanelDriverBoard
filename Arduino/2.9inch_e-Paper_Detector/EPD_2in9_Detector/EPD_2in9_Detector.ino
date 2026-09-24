/*****************************************************************************
* | File      	:   EPD_2in9_Detector.ino
* | Function    :   2.9inch black/white e-Paper version detector
* | Info        :   Tries every 2.9inch mono driver (V3, V2, V1, D, DES) in turn and
*                   writes on the panel which driver produced the image. The attempt
*                   that renders cleanly tells you which panel you own.
*                   Boards: SeeedStudio XIAO ESP32-S3/C3/C6 on the SeeedStudio ePaper Driver
*                   or ePaper Breakout board (same pins as epdif.h in the sibling sketches);
*                   Arduino UNO R4 Minima/WiFi, Waveshare ESP32 / ESP32-S3 (see DEV_Config.h).
*----------------
* |	This version:   V1.1
* | Date        :   2026-09-13
******************************************************************************/
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "EPD_2in9.h"
#include "EPD_2in9_V2.h"
#include "EPD_2in9_V3.h"
#include "EPD_2in9d.h"
#include "EPD_2in9_DES.h"

#define HOLD_MS          10000    /* how long each result stays on the panel */
#define INIT_TIMEOUT_MS   3000
#define REFRESH_TIMEOUT_MS 12000

enum Family { FAM_SSD, FAM_UC };

struct Attempt {
    const char *name;       /* what to print as the verdict */
    const char *chip;       /* chip family and waveform source */
    Family      fam;
    UWORD       rotate;     /* rotation used by the shipped example for this panel */
    void      (*init)(void);
    void      (*display)(UBYTE *);
    void      (*sleep)(void);
};

/* V1 Init takes a mode argument; wrap it so the table has one signature */
static void init_v1(void) { EPD_2IN9_Init(EPD_2IN9_FULL); }

static const Attempt SSD_GROUP[] = {
    {"2.9inch V3",  "SSD1680 OTP LUT (0x22=F7)",  FAM_SSD, ROTATE_90,  EPD_2IN9_V3_Init, EPD_2IN9_V3_Display_Base, EPD_2IN9_V3_Sleep},
    {"2.9inch V2",  "SSD1680 host LUT (0x22=C7)", FAM_SSD, ROTATE_90,  EPD_2IN9_V2_Init, EPD_2IN9_V2_Display_Base, EPD_2IN9_V2_Sleep},
    {"2.9inch V1",  "SSD1608/IL3820 host LUT",    FAM_SSD, ROTATE_270, init_v1,          EPD_2IN9_Display,         EPD_2IN9_Sleep},
};
static const Attempt UC_GROUP[] = {
    {"2.9inch D",   "UC8151 OTP, 0x10+0x13 planes", FAM_UC, ROTATE_270, EPD_2IN9D_Init,    EPD_2IN9D_Display,    EPD_2IN9D_Sleep},
    {"2.9inch DES", "UC8151 OTP, 0x13 plane only",  FAM_UC, ROTATE_270, EPD_2IN9_DES_Init, EPD_2IN9_DES_Display, EPD_2IN9_DES_Sleep},
};
#define N_SSD (sizeof(SSD_GROUP) / sizeof(SSD_GROUP[0]))
#define N_UC  (sizeof(UC_GROUP)  / sizeof(UC_GROUP[0]))
#define N_ALL (N_SSD + N_UC)

struct Result {
    const Attempt *a;
    uint32_t init_ms;
    uint32_t refresh_ms;
    uint8_t  init_timeouts;
    uint8_t  refresh_timeouts;
};

static UBYTE  frame[(EPD_2IN9_V2_WIDTH / 8) * EPD_2IN9_V2_HEIGHT];   /* 4736 bytes, reused */
static Result results[N_ALL];
static Family family;
static int    busyRaw, busyAfter71;

static const char *familyName(Family f) { return f == FAM_SSD ? "SSD" : "UC"; }

/* Probe: hardware reset, then read BUSY before and after a 0x71 (UC "get status").
   SSD168x idles LOW, UC8151 idles HIGH. Only decides which group is tried first. */
static void probeBusy(void)
{
    DEV_HW_Reset(200, 2);
    delay(200);
    busyRaw = digitalRead(EPD_BUSY_PIN);
    DEV_SendCommand(0x71);
    delay(10);
    busyAfter71 = digitalRead(EPD_BUSY_PIN);
    family = (busyAfter71 == HIGH) ? FAM_UC : FAM_SSD;

    Serial.print("Reset pulse 200/2/200 ms. BUSY after reset: ");
    Serial.print(busyRaw ? "HIGH" : "LOW");
    Serial.print(", after 0x71: ");
    Serial.println(busyAfter71 ? "HIGH" : "LOW");
    if (busyRaw != busyAfter71)
        Serial.println("  (samples differ: probe is ambiguous, both groups are tried anyway)");
    Serial.print("BUSY idle ");
    Serial.print(busyAfter71 ? "HIGH -> UC family (D/DES)" : "LOW -> SSD family (V1/V2/V3)");
    Serial.print(" likely. Trying ");
    Serial.print(familyName(family));
    Serial.println(" group first, then the other.");
}

static void drawLabel(int n, const Attempt *a)
{
    char line[48];

    Paint_DrawRectangle(1, 1, Paint.Width - 1, Paint.Height - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    Paint_DrawString_EN(6, 3, "2.9in e-Paper detector", &Font16, BLACK, WHITE);

    snprintf(line, sizeof line, "Try %d/%d: %s", n, (int)N_ALL, a->name);
    Paint_DrawString_EN(6, 22, line, &Font20, BLACK, WHITE);

    snprintf(line, sizeof line, "chip: %s", a->chip);
    Paint_DrawString_EN(6, 46, line, &Font12, BLACK, WHITE);

    snprintf(line, sizeof line, "BUSY idle %s -> %s group (%s)",
             busyAfter71 ? "HIGH" : "LOW",
             familyName(family),
             family == FAM_SSD ? "V1/V2/V3" : "D/DES");
    Paint_DrawString_EN(6, 60, line, &Font12, BLACK, WHITE);

    snprintf(line, sizeof line, "board: %s", EPD_BOARD_NAME);
    Paint_DrawString_EN(6, 74, line, &Font12, BLACK, WHITE);

    snprintf(line, sizeof line, "If readable: %s", a->name);
    Paint_DrawString_EN(6, 90, line, &Font16, WHITE, BLACK);

    /* N filled squares: countable even when the text is garbled */
    for (int i = 0; i < n; i++)
        Paint_DrawRectangle(8 + 16 * i, 110, 17 + 16 * i, 119, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

static void runAttempt(int n, const Attempt *a)
{
    Result *r = &results[n - 1];
    r->a = a;

    Serial.print("--- Attempt ");
    Serial.print(n);
    Serial.print("/");
    Serial.print((int)N_ALL);
    Serial.print(": ");
    Serial.print(a->name);
    Serial.print(" [");
    Serial.print(a->chip);
    Serial.println("]");

    DEV_ResetBusyStats();
    DEV_BusyTimeoutMs = INIT_TIMEOUT_MS;
    uint32_t t = millis();
    a->init();
    r->init_ms = millis() - t;
    r->init_timeouts = DEV_BusyTimeoutCount;
    Serial.print("  init    ");
    Serial.print(r->init_ms);
    Serial.print(" ms   busy timeouts: ");
    Serial.println(r->init_timeouts);

    Paint_NewImage(frame, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, a->rotate, WHITE);
    Paint_SelectImage(frame);
    Paint_Clear(WHITE);
    drawLabel(n, a);

    DEV_ResetBusyStats();
    DEV_BusyTimeoutMs = REFRESH_TIMEOUT_MS;
    t = millis();
    a->display(frame);
    r->refresh_ms = millis() - t;
    r->refresh_timeouts = DEV_BusyTimeoutCount;
    Serial.print("  refresh ");
    Serial.print(r->refresh_ms);
    Serial.print(" ms   busy timeouts: ");
    Serial.println(r->refresh_timeouts);

    Serial.print("  LOOK AT THE PANEL for ");
    Serial.print(HOLD_MS / 1000);
    Serial.print(" s. If the text is crisp and complete, your panel is: ");
    Serial.println(a->name);
    delay(HOLD_MS);

    DEV_BusyTimeoutMs = INIT_TIMEOUT_MS;
    a->sleep();
    Serial.println("  sleep");
}

static void printSummary(void)
{
    Serial.println();
    Serial.println("=== Summary ===");
    Serial.println(" # | driver       | family | init ms | refresh ms | timeouts");
    for (unsigned i = 0; i < N_ALL; i++) {
        const Result *r = &results[i];
        char line[80];
        snprintf(line, sizeof line, " %u | %-12s | %-6s | %7lu | %10lu | %u",
                 i + 1, r->a->name, familyName(r->a->fam),
                 (unsigned long)r->init_ms, (unsigned long)r->refresh_ms,
                 r->init_timeouts + r->refresh_timeouts);
        Serial.println(line);
    }
    Serial.print("BUSY idle says ");
    Serial.print(familyName(family));
    Serial.print(" family. Attempts with 0 timeouts:");
    bool any = false;
    for (unsigned i = 0; i < N_ALL; i++) {
        if (results[i].init_timeouts + results[i].refresh_timeouts == 0) {
            Serial.print(" ");
            Serial.print(i + 1);
            any = true;
        }
    }
    Serial.println(any ? "" : " none");
    Serial.println("V2 and V3 (and D vs DES) cannot be told apart electrically: pick the attempt whose text looked cleanest.");
    Serial.println("Done. Press reset to run again.");
}

void setup()
{
    DEV_Module_Init();
    while (!Serial && millis() < 3000) { }

    Serial.println();
    Serial.println("=== 2.9inch e-Paper mono panel detector ===");
    Serial.print("Board: ");
    Serial.print(EPD_BOARD_NAME);
    Serial.print("  pins SCK=");  Serial.print(EPD_SCK_PIN);
    Serial.print(" MOSI=");       Serial.print(EPD_MOSI_PIN);
    Serial.print(" CS=");         Serial.print(EPD_CS_PIN);
    Serial.print(" DC=");         Serial.print(EPD_DC_PIN);
    Serial.print(" RST=");        Serial.print(EPD_RST_PIN);
    Serial.print(" BUSY=");       Serial.print(EPD_BUSY_PIN);
#ifdef EPD_PWR_PIN
    Serial.print(" PWR=");        Serial.print(EPD_PWR_PIN);
#endif
    Serial.println();

    probeBusy();

    int n = 1;
    if (family == FAM_SSD) {
        for (unsigned i = 0; i < N_SSD; i++) runAttempt(n++, &SSD_GROUP[i]);
        for (unsigned i = 0; i < N_UC;  i++) runAttempt(n++, &UC_GROUP[i]);
    } else {
        for (unsigned i = 0; i < N_UC;  i++) runAttempt(n++, &UC_GROUP[i]);
        for (unsigned i = 0; i < N_SSD; i++) runAttempt(n++, &SSD_GROUP[i]);
    }

    printSummary();
    DEV_Module_Exit();    /* panel keeps the last image */
}

void loop()
{
}
