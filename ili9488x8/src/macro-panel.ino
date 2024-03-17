#include "TFT_eSPI.h"
#include <Adafruit_FT6206.h>
#include "Keyboard.h"
#include "LittleFS.h"
#include "tomlcpp.hpp"

#include <vector>
#include <string>

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ts = Adafruit_FT6206();

// 320x480
TFT_eSPI tft = TFT_eSPI();

// 320x480
#define BUTTON_X 64
#define BUTTON_Y 60
#define BUTTON_W 80
#define BUTTON_H 60
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 1

typedef struct {
    std::string name;
    std::string action;
    TFT_eSPI_Button *button;
    uint16_t color;
    bool display;
} keys_t;

const uint8_t rows= 4;
const uint8_t columns= 5;
keys_t buttons[rows][columns];

const char *deck1[rows][columns] = {
    {"Mac 0",  "Mac 1", "Mac 2", "Mac 3", "Mac 4"},
    {"Mac 5",  "Mac 6", "Mac 7", "Mac 8", "Mac 9"},
    {"Mac A",  "Mac B", "Mac C", "Mac D", "Mac E"},
    {"Mac F",  "Mac G", "Mac H", "Mac I", "Mac J"}
};

const char *deck2[rows][columns] = {
    {"Deck2 0", "Deck2 1", "Deck2 2", "Deck2 3", "Deck2 4"},
    {"Deck2 5", "Deck2 6", "Deck2 7", "Deck2 8", "Deck2 9"},
    {"Deck2 A", "Deck2 B", "Deck2 C", "Deck2 D",  nullptr},
    {"Deck2 F",  nullptr,  "Deck2 G",  nullptr,  "Deck2 I"}
};

void create_buttons()
{
    // create buttons
    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < columns; col++) {
            TFT_eSPI_Button *b= new TFT_eSPI_Button;
            buttons[row][col].button = b;
            buttons[row][col].name= deck1[row][col];
            buttons[row][col].action= buttons[row][col].name;
            buttons[row][col].color= TFT_BLUE;
            buttons[row][col].display= true;
            // x, y, w, h, outline, fill, text
            b->initButton(&tft, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                                BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                                BUTTON_W, BUTTON_H,
                                                TFT_WHITE,
                                                buttons[row][col].color, // button color
                                                TFT_WHITE,
                                                (char *)buttons[row][col].name.c_str(),
                                                BUTTON_TEXTSIZE);
            b->drawButton();
        }
    }
}

void update_buttons()
{
    // update buttons
    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < columns; col++) {
            if(buttons[row][col].display) {
                TFT_eSPI_Button *b= buttons[row][col].button;
                // x, y, w, h, outline, fill, text
                b->initButton(&tft, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                                    BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                                    BUTTON_W, BUTTON_H,
                                                    TFT_WHITE,
                                                    buttons[row][col].color, // button color
                                                    TFT_WHITE,
                                                    (char *)buttons[row][col].name.c_str(),
                                                    BUTTON_TEXTSIZE);
                b->drawButton();
            }
        }
    }
}

void swap_deck(const char *deck[][columns])
{
    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < columns; col++) {
            if(deck[row][col] != nullptr) {
                buttons[row][col].name= deck[row][col];
                buttons[row][col].display= true;
            }else{
                buttons[row][col].display = false;
            }
        }
    }
    update_buttons();
}

bool read_config()
{
    // read in config file and populate button structures
    LittleFS.begin();
    char buff[128];
    std::string str;
    bool ok= false;

    while(1) {
        File f = LittleFS.open("config.toml", "r");
        if (f) {
            int n;
            do {
                n = f.read((uint8_t *)buff, sizeof(buff));
                if(n > 0) {
                    str.append(buff, n);
                }
            } while (n > 0);

            f.close();

            // parse the config file
            auto res = toml::parse(str);
            if (!res.table) {
                Serial.printf("cannot parse file: %s\n", res.errmsg.c_str());
                break;
            }

            // iterate over each table which is a deck
            for (auto &i : res.table->keys()) {
                Serial.printf("%s\n", i.c_str());
                auto t = res.table->getTable(i);
                if(!t) {
                    Serial.printf("  Failed to get table\n");
                    continue;
                }
                {
                    auto s =  t->getString("name");
                    if(s.first) {
                        Serial.printf("  deck name = %s\n", s.second.c_str());
                    } else {
                        Serial.printf("  Failed to get deck name\n");
                        continue;
                    }
                }
                {
                    // read in the button array
                    auto s = t->getArray("buttons");
                    if(s) {
                        for (int n = 0; n < s->size(); ++n) {
                            auto tbl = s->getTable(n);
                            auto index = tbl->getInt("index");
                            auto name = tbl->getString("name");
                            auto text = tbl->getString("text");
                            if(!index.first || !name.first || !text.first) {
                                Serial.printf("    Error %d - missing index, name or text\n", n);
                                continue;
                            }
                            uint32_t idx = (uint32_t)index.second;

                            Serial.printf("    idx: %ld, name: %s, text: %s", idx, name.second.c_str(), text.second.c_str());
                            // optional
                            auto color = tbl->getString("color");
                            if(color.first) {
                                Serial.printf(", color: %s", color.second.c_str());
                            }
                            Serial.printf("\n");
                        }
                    } else {
                        Serial.printf("  Failed to get buttons\n");
                    }
                }
            }
            ok = true;

        } else {
            Serial.println("Failed to open config.toml");
            break;
        }

        break;
    }

    LittleFS.end();

    return ok;
}

void setup()
{
    Serial.begin(115200);
    // while (!Serial);

    if (!ts.begin(40)) {
        Serial.println("Unable to start touchscreen.");
    } else {
        Serial.println("Touchscreen started.");
    }

    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    if(!read_config()) {
        Serial.printf("Failed to read the config file\n");
    }

    create_buttons();

    Keyboard.begin();
}

static int n_fingers;
static TS_Point getScreenCoords()
{
    TS_Point p;
    n_fingers = ts.touched();

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
static bool was_down = false;
static int lst_y = 0;
static bool two_fingers = false;

void loop(void)
{
    TS_Point p = getScreenCoords();

    if(n_fingers > 1) {
        two_fingers = true;
        // detect swipe up or down
        if(p.z) {
            if(was_down) {
                int dy = lst_y - p.y;
                if(abs(dy) > tft.height() / 2) {
                    // swipe detected
                    Serial.printf("Swiped %d\n", dy);
                    lst_y = p.y;
                    tft.fillScreen(TFT_BLACK);
                    if(dy > 0) {
                        // swiped up
                        swap_deck(deck1);

                    } else {
                        // swiped down
                        swap_deck(deck2);
                    }
                    return;
                }
            } else {
                was_down = true;
                lst_y = p.y;
            }

        } else {
            was_down = false;
        }
    }

    if(two_fingers) {
        if(p.z == 0) {
            two_fingers = false;
            was_down = false;
        } else {
            return;
        }
    }

    // go thru all the buttons, checking if they were pressed
    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < columns; col++) {
            if(!buttons[row][col].display) continue;
            TFT_eSPI_Button *b= buttons[row][col].button;
            if (b->contains(p.x, p.y)) {
                if(!b->isPressed()) {
                    Serial.printf("Pressing: %d\n", b);
                    b->press(true);  // tell the button it is pressed
                }
            } else {
                b->press(false);  // tell the button it is NOT pressed
            }
        }
    }

    // now we can ask the buttons if their state has changed
    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < columns; col++) {
            if(!buttons[row][col].display) continue;
            TFT_eSPI_Button *b= buttons[row][col].button;
            if (b->justReleased()) {
                Serial.printf("Released: %d\n", b);
                b->drawButton();  // draw normal
                Keyboard.write((const uint8_t*)buttons[row][col].action.c_str(), buttons[row][col].action.size());

            } else if (b->justPressed()) {
                b->drawButton(true);  // draw invert!
            }
        }
    }

    delay(100);
}
