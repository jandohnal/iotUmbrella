#include "OledDisplay.h"

OledDisplay::OledDisplay() : display(U8G2_R0, U8X8_PIN_NONE) {}

bool OledDisplay::Initialize() {
  return display.begin();
}

void OledDisplay::ShowSplash() {
  display.clearBuffer();
  display.setFont(u8g2_font_profont22_mf);
  display.setCursor(0, 20);
  display.print("Tank meter");
  display.setFont(u8g2_font_profont12_mf);
  display.setCursor(0, 44);
  display.print("cekam na data...");
  display.sendBuffer();
}

void OledDisplay::ShowReading(int distanceCm, int volume) {
  display.clearBuffer();
  display.setFont(u8g2_font_profont22_mf);
  display.setCursor(0, 20);
  display.print("L: ");
  display.print(distanceCm);
  display.print(" cm");
  display.setCursor(0, 44);
  display.print("V: ");
  display.print(volume);
  display.print(" l");
  display.sendBuffer();
}

void OledDisplay::ShowError(const char* msg) {
  display.clearBuffer();
  display.setFont(u8g2_font_profont22_mf);
  display.setCursor(0, 32);
  display.print(msg);
  display.sendBuffer();
}
