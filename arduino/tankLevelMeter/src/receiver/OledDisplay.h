#pragma once
#include <U8g2lib.h>

// OLED 128x64 (SSD1306) přes hardwarové I2C.
class OledDisplay {
  private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

  public:
    OledDisplay();

    bool Initialize();
    void ShowSplash();
    void ShowReading(int distanceCm, int volume);
    void ShowError(const char* msg);
};
