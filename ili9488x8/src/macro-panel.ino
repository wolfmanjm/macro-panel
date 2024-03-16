#include "TFT_eSPI.h"
#include <Adafruit_FT6206.h>

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ts = Adafruit_FT6206();

// 320x480
TFT_eSPI tft = TFT_eSPI();

// 320x480
#define BUTTON_X 78
#define BUTTON_Y 60
#define BUTTON_W 100
#define BUTTON_H 60
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 2

char buttonlabels[16][8];

const char *deck1[16] = {
    "Mac 1", "Mac 2", "Mac 3", "Mac 4",
    "Mac 5", "Mac 6", "Mac 7", "Mac 8",
    "Mac 9", "Mac A", "Mac B", "Mac C",
    "Mac D", "Mac E", "Mac F", "Mac 0"
};

const char *deck2[16] = {
    "Deck2 1", "Deck2 2", "Deck2 3", "Deck2 4",
    "Deck2 5", "Deck2 6", "Deck2 7", "Deck2 8",
    "Deck2 9", "Deck2 A", "Deck2 B", "Deck2 C",
    "Deck2 D", "Deck2 E", "Deck2 F", "Deck2 0"
};

uint16_t buttoncolors[16] = {TFT_DARKGREEN, TFT_DARKGREY, TFT_RED,
                             TFT_RED, TFT_BLUE, TFT_BLUE,
                             TFT_BLUE, TFT_BLUE, TFT_BLUE,
                             TFT_BLUE, TFT_BLUE, TFT_BLUE,
                             TFT_BLUE, TFT_BLUE, TFT_BLUE,
                             TFT_BLUE
                            };
TFT_eSPI_Button buttons[16];

void create_buttons()
{
    // create buttons
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            // x, y, w, h, outline, fill, text
            buttons[col + (row * 4)].initButton(&tft, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                                BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                                BUTTON_W, BUTTON_H,
                                                TFT_WHITE,
                                                buttoncolors[col + (row * 4)],
                                                TFT_WHITE,
                                                buttonlabels[col + (row * 4)],
                                                BUTTON_TEXTSIZE);
            buttons[col + (row * 4)].drawButton();
        }
    }
}

void swap_deck(const char **deck)
{
    for (int i = 0; i < 16; ++i) {
        strncpy(buttonlabels[i], deck[i], 7);
    }
}

void setup()
{
    Serial.begin(115200);
    //while (!Serial);

    if (!ts.begin(40)) {
        Serial.println("Unable to start touchscreen.");
    } else {
        Serial.println("Touchscreen started.");
    }

    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    swap_deck(deck1);
    create_buttons();
}

static int n_fingers;
static TS_Point getScreenCoords()
{
    TS_Point p;
    n_fingers= ts.touched();

    if (n_fingers > 0) {
        // Retrieve a point
        TS_Point np = ts.getPoint();
        // Serial.printf("x= %d, y= %d, z= %d\n", p.x, p.y, p.z);

        // rotate coordinate system
        // flip it around to match the screen.
        int w = 320; // touch coords
        int h = 480;
        p.y = np.x;
        p.x = h - np.y;
        p.z = 1;
    } else {
        p.x = -1;
        p.y = -1;
        p.z = 0;
    }

    return p;
}

// used for swipe detection
static bool was_down= false;
static int lst_y= 0;
static bool two_fingers= false;

void loop(void)
{
    TS_Point p= getScreenCoords();

    if(n_fingers > 1) {
        two_fingers= true;
        // detect swipe up or down
        if(p.z) {
            if(was_down) {
                int dy = lst_y - p.y;
                if(abs(dy) > tft.height()/2) {
                    // swipe detected
                    Serial.printf("Swiped %d\n", dy);
                    lst_y = p.y;
                    if(dy > 0) {
                        // swiped up
                        swap_deck(deck1);
                    } else{
                        // swiped down
                        swap_deck(deck2);
                    }
                    create_buttons();
                }
            }else{
                was_down= true;
                lst_y = p.y;
            }

        }else{
            was_down= false;
        }
    }

    if(two_fingers) {
        if(p.z == 0) {
            two_fingers= false;
            was_down= false;
        }else{
            return;
        }
    }

    // go thru all the buttons, checking if they were pressed
    for (uint8_t b = 0; b < 16; b++) {
        if (buttons[b].contains(p.x, p.y)) {
            if(!buttons[b].isPressed()) {
                Serial.printf("Pressing: %d\n", b);
                buttons[b].press(true);  // tell the button it is pressed
            }
        } else {
            buttons[b].press(false);  // tell the button it is NOT pressed
        }
    }

    // now we can ask the buttons if their state has changed
    for (uint8_t b = 0; b < 16; b++) {
        if (buttons[b].justReleased()) {
            Serial.printf("Released: %d\n", b);
            buttons[b].drawButton();  // draw normal
        }

        if (buttons[b].justPressed()) {
            buttons[b].drawButton(true);  // draw invert!
        }
    }

    delay(100);
}
