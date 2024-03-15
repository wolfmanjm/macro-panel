#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it

#ifdef TEENSY41
#include <ILI9341_t3.h>
// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST 23
#define TFT_CS 10
#define TFT_DC  9
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);

#else
// arduino micro pro
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST 23
#define TFT_CS 10
#define TFT_DC  9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#endif

#include <XPT2046_Touchscreen.h>

// This is calibration data for the raw touch data to the screen coordinates
const float xCalM = -0.09, xCalC = 336.68, yCalM = -0.07, yCalC = 257.70;

#define ROTATE 3

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
XPT2046_Touchscreen ts = XPT2046_Touchscreen(STMPE_CS);


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
                            "Mac D", "Mac E", "Mac F", "Mac 0" };

uint16_t buttoncolors[16] = {ILI9341_DARKGREEN, ILI9341_DARKGREY, ILI9341_RED,
                             ILI9341_RED, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE, ILI9341_BLUE, ILI9341_BLUE,
                             ILI9341_BLUE
                            };
Adafruit_GFX_Button buttons[16];

void setup(void)
{
    Serial.begin(115200);
    Serial.println("macro panel");

    tft.begin();

    if (!ts.begin()) {
        Serial.println("Couldn't start touchscreen controller");
        while (1);
    }
    Serial.println("Touchscreen started");

    ts.setRotation(ROTATE);
    tft.setRotation(ROTATE);
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
}


void loop()
{
    TS_Point p;

    // See if there's any  touch data for us
    if (ts.bufferEmpty()) {
        return;
    }


    // see if any pressure
    if(ts.touched()) {
        // yes
        p = getScreenCoords();
        // Serial.println("touched");
    } else {
        // no
        p.x = -1;
        p.y = -1;
    }

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
