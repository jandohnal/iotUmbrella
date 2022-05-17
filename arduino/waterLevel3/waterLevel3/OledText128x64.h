#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>

//screen definition
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

class OledText128x64
{
    //adafruit display
    public: Adafruit_SSD1306 display;

    public:
    //constructor
    OledText128x64(int i);

    //display initialization
    bool Initialize();

    //display four rows of values
    // l - length
    // v - volume
    // t - temperature (float)
    // h - humidity (float)
    void Display(int l, int v, int t, int h);

    //show adafruit splashscreen logo
    void ShowSplash();
};