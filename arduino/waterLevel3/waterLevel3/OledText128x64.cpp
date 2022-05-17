#include "OledText128x64.h"


OledText128x64::OledText128x64(int i) : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
{
}

bool OledText128x64::Initialize()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            return false;
    }
    return true;
}

void OledText128x64::Display(int l, int v, int t, int h)
{
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    // String dist("L: "); dist += l; dist += " cm";
    // String vol("V: "); vol += v; vol += " l";
    // String temp("T: "); temp += t; temp += " C";
    // String humidity("H: "); humidity += t; humidity += " %";

    display.print("L: "); display.print(l); display.println(" cm");
    display.print("V: "); display.print(v); display.println(" l");
    display.print("T: ");display.print(t);display.println(" C");
    display.print(h);display.println(" %");
    display.display();
}

void OledText128x64::ShowSplash()
{
    display.clearDisplay();
    display.display();
}