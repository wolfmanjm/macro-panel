#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
//#include "tinyflash.h"


#ifdef TEENSY41
#include <ILI9341_t3.h>
// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST 23
#define TFT_CS 10
#define TFT_DC  9
#define T_CS 8
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);

#elif defined(MICROPRO)

// arduino micro pro
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST 23
#define TFT_CS 10
#define TFT_DC  9
#define T_CS 8
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#elif defined(PICO)
#include "TFT_eSPI.h"

// #define SCLK 2
// #define MOSI 7 // 3
// #define MISO 4
// #define TFT_CS 5
// #define TFT_DC  6
// #define TFT_RST 10 // 7
//#define T_CS 8
#define FLASH_CS 9
TFT_eSPI tft = TFT_eSPI();


#else
#error "Not a recognized board"
#endif

#ifndef PICO
#include <XPT2046_Touchscreen.h>
// This is calibration data for the raw touch data to the screen coordinates
const float xCalM = -0.09, xCalC = 336.68, yCalM = -0.07, yCalC = 257.70;
XPT2046_Touchscreen ts = XPT2046_Touchscreen(T_CS);

static TS_Point getScreenCoords()
{
    // Retrieve a point
    TS_Point p = ts.getPoint();

    int xCoord = round((p.x * xCalM) + xCalC);
    int yCoord = round((p.y * yCalM) + yCalC);
    if(xCoord < 0) xCoord = 0;
    if(xCoord >= tft.width()) xCoord = tft.width() - 1;
    if(yCoord < 0) yCoord = 0;
    if(yCoord >= tft.height()) yCoord = tft.height() - 1;
    p.x = xCoord;
    p.y = yCoord;
    return p;
}

#else

typedef struct {
    int x;
    int y;
} TS_Point;

static TS_Point getScreenCoords()
{
    // Retrieve a point
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
    // Pressed will be set true is there is a valid touch on the screen
    bool pressed = tft.getTouch(&t_x, &t_y);
    TS_Point p;
    if(pressed) {
        p.x = t_x;
        p.y = t_y;
    } else {
        p.x = -1;
        p.y = -1;
    }
    return p;
}

// uncomment to do touch calibration
//#define CALIBRATE

static uint16_t calData[5] = {450, 3264, 384, 3275, 1};
#ifdef CALIBRATE
static void touch_calibrate()
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // TODO save somewhere
    for (int i = 0; i < 5; ++i) {
        Serial.printf("%d, ", calData[i]);
    }
    Serial.println();
}
#endif

#endif

#define ROTATE 3
// 320x240
#define BUTTON_X 38
#define BUTTON_Y 34
#define BUTTON_W 70
#define BUTTON_H 50
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 1

/* create 16 buttons, in 4x4 grid */
#ifdef TEENSY41
const char *buttonlabels[16] =
#else
char buttonlabels[16][8] =
#endif
{
    "Mac 1", "Mac 2", "Mac 3", "Mac 4",
    "Mac 5", "Mac 6", "Mac 7", "Mac 8",
    "Mac 9", "Mac A", "Mac B", "Mac C",
    "Mac D", "Mac E", "Mac F", "Mac 0"
};

uint16_t buttoncolors[16] = {ILI9341_DARKGREEN, ILI9341_DARKGREY, ILI9341_RED,
                             ILI9341_RED, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE
                            };

#ifdef PICO
TFT_eSPI_Button buttons[16];
#else
Adafruit_GFX_Button buttons[16];
#endif

void setup(void)
{
    Serial.begin(115200);
    //while (!Serial);
    Serial.println("Starting macro panel");

#ifdef PICO
    // SPI.setRX(MISO);
    // SPI.setTX(MOSI);
    // SPI.setSCK(SCLK);
    // SPI.setCS(TFT_CS);
    tft.init();
#else
    tft.begin();
    if (!ts.begin()) {
        Serial.println("Couldn't start touchscreen controller");
        while (1);
    }
    Serial.println("Touchscreen started");
    ts.setRotation(ROTATE);
#endif

    tft.setRotation(ROTATE);

#ifdef PICO
#ifdef CALIBRATE
    Serial.println("Doing calibration");
    touch_calibrate();
#endif
    tft.setTouch(calData);
#endif
    tft.fillScreen(ILI9341_BLACK);

    // create buttons
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            // x, y, w, h, outline, fill, text
            buttons[col + (row * 4)].initButton(&tft, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                                BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                                BUTTON_W, BUTTON_H,
                                                ILI9341_WHITE,
                                                buttoncolors[col + (row * 4)],
                                                ILI9341_WHITE,
                                                buttonlabels[col + (row * 4)],
                                                BUTTON_TEXTSIZE);
            buttons[col + (row * 4)].drawButton();
        }
    }

    Serial.println("Setup complete");
}

void loop()
{
    TS_Point p;

#ifndef PICO
    // See if there's any  touch data for us
    if (ts.bufferEmpty()) {
        return;
    }

    // see if any pressure
    if(ts.touched()) {
        // yes
        p = getScreenCoords();
        Serial.println("touched");
    } else {
        // no
        p.x = -1;
        p.y = -1;
    }

#else
    p = getScreenCoords();
    // Serial.printf("x= %d, y= %d\n", p.x, p.y);
#endif

    // go thru all the buttons, checking if they were pressed
    for (uint8_t b = 0; b < 16; b++) {
        if (buttons[b].contains(p.x, p.y)) {
            Serial.print("Pressing: "); Serial.println(b);
            buttons[b].press(true);  // tell the button it is pressed
        } else {
            buttons[b].press(false);  // tell the button it is NOT pressed
        }
    }

    // now we can ask the buttons if their state has changed
    for (uint8_t b = 0; b < 16; b++) {
        if (buttons[b].justReleased()) {
            Serial.print("Released: "); Serial.println(b);
            buttons[b].drawButton();  // draw normal
        }

        if (buttons[b].justPressed()) {
            buttons[b].drawButton(true);  // draw invert!
        }
    }

    delay(100);
}


