#include "TFT_eSPI.h"
#include <Adafruit_FT6206.h>
#include "Keyboard.h"
#include "LittleFS.h"
#include "tomlcpp.hpp"

#include <vector>
#include <string>

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ts = Adafruit_FT6206();
TFT_eSPI tft = TFT_eSPI();

// 320x480 for 5 columns by 4 rows of buttons
#define BUTTON_X 64
#define BUTTON_Y 60
#define BUTTON_W 80
#define BUTTON_H 60
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 1

typedef struct {
    TFT_eSPI_Button button;
    std::string name;
    std::string action;
    uint16_t color;
    uint8_t row, col;
    uint8_t text_size;
} keys_t;

const uint8_t nrows= 4;
const uint8_t ncolumns= 5;

// points to current deck
std::vector<keys_t> *buttons;

#define NDECKS 2
std::vector<keys_t> decks[NDECKS];
int current_deck= 0;

void update_buttons()
{
    // update buttons
    for( auto &b : *buttons) {
        uint8_t row= b.row;
        uint8_t col= b.col;
        // x, y, w, h, outline, fill, text, textsize
        b.button.initButton(&tft, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                                BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                                BUTTON_W, BUTTON_H,
                                                TFT_WHITE,
                                                b.color, // button color
                                                TFT_WHITE,
                                                (char *)b.name.c_str(),
                                                b.text_size);
        b.button.drawButton();
    }
}

void swap_deck(int d)
{
    if(d >= NDECKS) d= NDECKS-1;
    else if(d < 0) d= 0;

    buttons= &decks[d];
    current_deck= d;
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
                            auto row = tbl->getInt("row");
                            auto col = tbl->getInt("col");
                            auto name = tbl->getString("name");
                            auto action = tbl->getString("action");
                            if(!row.first || !col.first || !name.first || !action.first) {
                                Serial.printf("    Error %d - missing row, col, name or action\n", n);
                                continue;
                            }
                            uint32_t r = (uint32_t)row.second;
                            uint32_t c = (uint32_t)col.second;

                            if(r >= nrows || c >= ncolumns) {
                                Serial.printf("    Error %d - row or col out of range\n", n);
                                continue;
                            }
                            Serial.printf("    row: %ld, col: %ld, name: %s, action: %s", r, c, name.second.c_str(), action.second.c_str());

                            // setup the key data
                            keys_t key;
                            key.row = r;
                            key.col = c;
                            key.name = name.second;
                            key.action = action.second;
                            key.color = TFT_BLUE;
                            key.text_size = BUTTON_TEXTSIZE;

                            // optional
                            auto color = tbl->getString("color");
                            if(color.first) {
                                Serial.printf(", color: %s", color.second.c_str());
                                if(color.second == "red") key.color = TFT_RED;
                                else if(color.second == "blue") key.color = TFT_BLUE;
                                else if(color.second == "green") key.color = TFT_GREEN;
                                else {
                                    Serial.printf("    Warning unknown color: %s", color.second.c_str());
                                    key.color = TFT_BLUE;
                                }
                            }
                            auto ts = tbl->getInt("text_size");
                            if(ts.first) {
                                Serial.printf(", text_size: %ld", (uint32_t)ts.second);
                                key.text_size = ts.second;
                            }

                            Serial.printf("\n");

                            // store in current deck
                            decks[current_deck].push_back(key);
                        }

                    } else {
                        Serial.printf("  Failed to get buttons\n");
                    }
                }
                if(++current_deck >= NDECKS) break;
            }
            ok = true;
            current_deck= 0;

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

    // setup from config. This will load the decks
    if(!read_config()) {
        Serial.printf("Failed to read the config file\n");
    }

    // use data from first deck
    swap_deck(current_deck);

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
                if(abs(dy) > tft.height() / 4) {
                    // swipe detected
                    Serial.printf("Swiped %d\n", dy);
                    lst_y = p.y;
                    tft.fillScreen(TFT_BLACK);
                    if(dy > 0) {
                        // swiped up
                        swap_deck(--current_deck);
                    } else {
                        // swiped down
                        swap_deck(++current_deck);
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
    for( auto &b : *buttons) {
        if (b.button.contains(p.x, p.y)) {
            if(!b.button.isPressed()) {
                Serial.printf("Pressing: %s\n", b.name.c_str());
                b.button.press(true);  // tell the button it is pressed
            }
        } else {
            b.button.press(false);  // tell the button it is NOT pressed
        }
    }

    // now we can ask the buttons if their state has changed
    for( auto &b : *buttons) {
        if (b.button.justReleased()) {
            Serial.printf("Released: %s\n", b.name.c_str());
            b.button.drawButton();  // draw normal
            Keyboard.write((const uint8_t*)b.action.c_str(), b.action.size());
        } else if (b.button.justPressed()) {
            b.button.drawButton(true);  // draw invert!
        }
    }

    delay(100);
}
