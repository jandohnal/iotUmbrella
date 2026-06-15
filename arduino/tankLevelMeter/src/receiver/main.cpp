// =====================================================================
//  PŘIJÍMAČ – ESP32
//  nRF24L01 + OLED displej. Přijímá surovou vzdálenost ze senzoru,
//  počítá objem vody v nádrži a zobrazuje ho. Veškerá logika je zde.
// =====================================================================
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <TankProtocol.h>
#include "WaterTank.h"
#include "OledDisplay.h"

// --- Piny nRF24L01 (VSPI: SCK=18, MISO=19, MOSI=23) ---
#define RF_CE_PIN   4
#define RF_CSN_PIN  5

static const int TANK_VOLUME = 6000;
static const int TANK_HEIGHT = 145;

// Parametry nádrže – přesunuto z původního sketche.
//   výška vodního sloupce 145 cm, objem 6000 l, offset senzoru 58 cm.
WaterTank tank(TANK_HEIGHT, TANK_VOLUME, 24);
OledDisplay display;
RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Po jak dlouhém tichu hlásit ztrátu signálu.
static const unsigned long SIGNAL_TIMEOUT_MS = 180000;  // 3 min
unsigned long lastRx = 0;
bool signalLost = false;

void setup() {
  Serial.begin(115200);

  bool ok = radio.begin();
  
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);     // explicitně – musí sedět s vysílačem
  radio.setChannel(TANK_RF_CHANNEL);
  radio.openReadingPipe(1, TANK_RF_ADDRESS);
  radio.startListening();

  Serial.print(F("radio.begin(): "));     Serial.println(ok ? F("OK") : F("FAIL"));
  Serial.print(F("isChipConnected(): ")); Serial.println(radio.isChipConnected() ? F("ANO") : F("NE - zkontroluj SPI/napajeni!"));

  if (!display.Initialize()) {
    Serial.println(F("SSD1306 init failed"));
    for (;;);
  }
  display.ShowSplash();
  lastRx = millis();
}

void loop() {
  if (radio.available()) {
    TankReading reading;
    radio.read(&reading, sizeof(reading));
    lastRx = millis();
    signalLost = false;

    Serial.print(F("dist="));
    Serial.print(reading.distanceCm);
    Serial.print(F("cm  batt="));
    Serial.print(reading.batteryMv);
    Serial.print(F("mV  status="));
    Serial.println(reading.status);

    if (reading.status == TANK_NO_ECHO) {
      display.ShowError("No echo");
    } else {
      int volume = tank.GetActVolume(reading.distanceCm);
      const char* status = "";

      if (volume == 0) {
        status = "Sucho";
      } else if (volume > TANK_VOLUME) {
        status = "Nad kótou přelivu";
      }

      display.ShowReading(reading.distanceCm, volume, status);
    }
  }

  if (!signalLost && millis() - lastRx > SIGNAL_TIMEOUT_MS) {
    display.ShowError("No signal");
    signalLost = true;
  }
}
