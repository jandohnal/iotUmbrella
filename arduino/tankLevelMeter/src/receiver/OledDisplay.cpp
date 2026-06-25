#include "OledDisplay.h"
#include <TankProtocol.h>   // TANK_TEMP_INVALID

OledDisplay::OledDisplay() : display(U8G2_R0, U8X8_PIN_NONE) {}

bool OledDisplay::Initialize()
{
  bool ok = display.begin();
  display.enableUTF8Print();   // umožní tisk české diakritiky (UTF-8 řetězce)

  return ok;
}

void OledDisplay::ShowSplash()
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvB14_te);
  display.setCursor(0, 18);
  display.print("Tank meter");
  display.setFont(u8g2_font_helvB10_te);
  display.setCursor(0, 44);
  display.print("čekám na data...");
  display.sendBuffer();
}

void OledDisplay::ShowReading(int distanceCm, int volume, const char* msg)
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvB14_te);
  display.setCursor(0, 16);
  display.print("L: ");
  display.print(distanceCm);
  display.print(" cm");
  display.setCursor(0, 38);
  display.print("V: ");
  display.print(volume);
  display.print(" l");
  display.setFont(u8g2_font_helvB10_te);
  display.setCursor(0, 60);     // poslední řádek se vejde na 64px displej
  display.print(msg);
  display.sendBuffer();
}

void OledDisplay::ShowGraph(const int8_t* buf, int capacity, int head, int count)
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvR08_te);
  display.setCursor(0, 9);
  display.print("Hladina 24h");

  // Plocha grafu (rámeček).
  const int x0   = 0;
  const int x1   = 127;
  const int yTop = 14;
  const int yBot = 63;
  display.drawFrame(x0, yTop, x1 - x0 + 1, yBot - yTop + 1);

  if (count <= 0)
  {
    display.setCursor(18, 42);
    display.print("zatím bez dat");
    display.sendBuffer();
    return;
  }

  // Aktuální hodnota (poslední vzorek) vpravo nahoře.
  int lastIdx = (head - 1 + capacity) % capacity;
  display.setCursor(96, 9);
  display.print(buf[lastIdx]);
  display.print("%");

  // Vykreslení průběhu uvnitř rámečku (1px okraj).
  const int plotW = (x1 - x0) - 2;     // šířka kreslicí oblasti
  const int plotH = (yBot - yTop) - 2; // výška kreslicí oblasti
  int prevX = -1, prevY = -1;
  for (int i = 0; i < count; i++)
  {
    int idx = (head - count + i + capacity) % capacity;
    int pct = buf[idx];
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    int x = x0 + 1 + (count == 1 ? 0 : (int)((long)i * plotW / (count - 1)));
    int y = yBot - 1 - (int)((long)pct * plotH / 100);

    if (prevX >= 0) display.drawLine(prevX, prevY, x, y);
    else            display.drawPixel(x, y);
    prevX = x;
    prevY = y;
  }
  display.sendBuffer();
}

void OledDisplay::ShowSensor(int temperatureC10, int humidityRh10)
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvR08_te);
  display.setCursor(0, 9);
  display.print("Senzor SHT40");

  if (temperatureC10 == TANK_TEMP_INVALID)
  {
    display.setFont(u8g2_font_helvB10_te);
    display.setCursor(0, 40);
    display.print("čidlo nečte");
    display.sendBuffer();
    return;
  }

  display.setFont(u8g2_font_helvB14_te);

  // Teplota (se znaménkem a jedním desetinným místem, ať se neztratí -0,x °C).
  int absT = temperatureC10 < 0 ? -temperatureC10 : temperatureC10;
  display.setCursor(0, 34);
  display.print("T: ");
  if (temperatureC10 < 0) display.print("-");
  display.print(absT / 10);
  display.print(".");
  display.print(absT % 10);
  display.print(" °C");

  // Vlhkost (0–100 %RH, vždy kladná).
  display.setCursor(0, 60);
  display.print("H: ");
  display.print(humidityRh10 / 10);
  display.print(".");
  display.print(humidityRh10 % 10);
  display.print(" %");

  display.sendBuffer();
}

void OledDisplay::ShowDiagnostics(bool wifiConnected, int rssi, const char* ip)
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvB10_te);
  display.setCursor(0, 12);
  display.print("Diagnostika");

  display.setFont(u8g2_font_helvR08_te);

  display.setCursor(0, 30);
  display.print("WiFi: ");
  display.print(wifiConnected ? "připojeno" : "odpojeno");

  display.setCursor(0, 44);
  display.print("Signál: ");
  if (wifiConnected)
  {
    display.print(rssi);
    display.print(" dBm");
  }
  else
  {
    display.print("-");
  }

  display.setCursor(0, 58);
  display.print("IP: ");
  display.print(wifiConnected ? ip : "-");

  display.sendBuffer();
}

void OledDisplay::ShowError(const char* msg)
{
  display.clearBuffer();
  display.setFont(u8g2_font_helvB14_te);
  display.setCursor(0, 38);
  display.print(msg);
  display.sendBuffer();
}
