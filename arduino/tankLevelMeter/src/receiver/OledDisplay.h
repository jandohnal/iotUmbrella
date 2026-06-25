#pragma once
#include <U8g2lib.h>

// OLED 128x64 (SSD1306) přes hardwarové I2C.
class OledDisplay
{
  private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

  public:
    OledDisplay();

    bool Initialize();
    void ShowSplash();
    void ShowReading(int distanceCm, int volume, const char* msg);

    // Graf historie naplnění [%] za posledních 24 h. Data jsou v kruhovém
    // bufferu:
    //   buf      – pole hodnot 0–100 [%]
    //   capacity – velikost pole
    //   head     – index, kam se zapíše příští vzorek
    //   count    – počet platných vzorků (<= capacity)
    void ShowGraph(const int8_t* buf, int capacity, int head, int count);

    // Obrazovka THP senzoru (SHT40): teplota a vlhkost.
    //   temperatureC10 – teplota [0,1 °C], TANK_TEMP_INVALID = senzor nečte
    //   humidityRh10   – vlhkost [0,1 %RH]
    void ShowSensor(int temperatureC10, int humidityRh10);

    // Diagnostická obrazovka: stav WiFi, síla signálu a IP adresa.
    void ShowDiagnostics(bool wifiConnected, int rssi, const char* ip);

    void ShowError(const char* msg);
};
