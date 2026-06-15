#include "OledDisplay.h"

OledDisplay::OledDisplay() : display(U8G2_R0, U8X8_PIN_NONE) {}

bool OledDisplay::Initialize() {
  bool ok = display.begin();
  display.enableUTF8Print();   // umožní tisk české diakritiky (UTF-8 řetězce)
  return ok;
}

void OledDisplay::ShowSplash() {
  display.clearBuffer();
  display.setFont(u8g2_font_helvB14_te);
  display.setCursor(0, 18);
  display.print("Tank meter");
  display.setFont(u8g2_font_helvB10_te);
  display.setCursor(0, 44);
  display.print("čekám na data...");
  display.sendBuffer();
}

void OledDisplay::ShowReading(int distanceCm, int volume, const char* msg) {
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

void OledDisplay::ShowError(const char* msg) {
  display.clearBuffer();
  display.setFont(u8g2_font_helvB14_te);
  display.setCursor(0, 38);
  display.print(msg);
  display.sendBuffer();
}
