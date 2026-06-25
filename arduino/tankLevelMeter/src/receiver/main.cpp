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
#include "HomeAssistantMqtt.h"

// --- Piny nRF24L01 (VSPI: SCK=18, MISO=19, MOSI=23) ---
#define RF_CE_PIN   4
#define RF_CSN_PIN  5

// Tlačítko pro přepínání obrazovek displeje. Zapojení: pin -> tlačítko -> GND
// (využívá interní pull-up, takže stisk = LOW).
#define BUTTON_PIN  27

static const int TANK_VOLUME = 6000;
static const int TANK_HEIGHT = 145;

// Parametry nádrže – přesunuto z původního sketche.
//   výška vodního sloupce 145 cm, objem 6000 l, offset senzoru 58 cm.
WaterTank tank(TANK_HEIGHT, TANK_VOLUME, 24);
OledDisplay display;
RF24 radio(RF_CE_PIN, RF_CSN_PIN);
HomeAssistantMqtt ha;

// Po jak dlouhém tichu hlásit ztrátu signálu.
static const unsigned long SIGNAL_TIMEOUT_MS = 180000;  // 3 min
unsigned long lastRx = 0;
bool signalLost = false;

// --- Přepínání obrazovek tlačítkem ---
//   0 = měření, 1 = graf, 2 = diagnostika, 3 = THP senzor (teplota/vlhkost)
static const int SCREEN_COUNT = 4;
int screenIndex = 0;
int buttonState = HIGH;
unsigned long lastButtonChange = 0;
static const unsigned long DEBOUNCE_MS = 50;

// Poslední naměřené hodnoty – ať lze obrazovku překreslit i bez nových dat
// (např. po stisku tlačítka).
bool        haveReading = false;
bool        lastNoEcho  = false;
int         lastDistance = 0;
int         lastVolume   = 0;
int         lastPercent  = 0;
const char* lastStatus   = "";
int         lastTemperatureC10 = TANK_TEMP_INVALID;
int         lastHumidityRh10   = 0;

// Kruhový buffer historie naplnění [%] pro graf "hladina za 24 h".
// 120 vzorků / 24 h => 1 vzorek každých 12 minut (plovoucí okno).
static const int HIST_SIZE = 120;
int8_t        history[HIST_SIZE];
int           histHead  = 0;   // index pro příští zápis
int           histCount = 0;   // počet platných vzorků
unsigned long lastSample = 0;
static const unsigned long SAMPLE_INTERVAL_MS = 24UL * 3600UL * 1000UL / HIST_SIZE;

// Vykreslí aktuálně zvolenou obrazovku z uložených hodnot.
void renderScreen()
{
  switch (screenIndex)
  {
    case 0:   // měření (vzdálenost, objem)
      if (signalLost)        display.ShowError("No signal");
      else if (!haveReading) display.ShowSplash();
      else if (lastNoEcho)   display.ShowError("No echo");
      else                   display.ShowReading(lastDistance, lastVolume, lastStatus);
      break;

    case 1:   // graf hladiny za 24 h
      display.ShowGraph(history, HIST_SIZE, histHead, histCount);
      break;

    case 2:   // diagnostika WiFi
      display.ShowDiagnostics(ha.IsWifiConnected(), ha.WifiRssi(), ha.WifiIp().c_str());
      break;

    case 3:   // THP senzor (teplota / vlhkost ze SHT40)
      if (!haveReading) display.ShowSplash();
      else              display.ShowSensor(lastTemperatureC10, lastHumidityRh10);
      break;
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Banner + důvod posledního resetu – pomáhá poznat, jestli se ESP32
  // necyklí (brownout = podpětí, panic = pád apod.).
  Serial.print(F("\n=== BOOT === reset_reason="));
  Serial.println((int)esp_reset_reason());

  bool ok = radio.begin();

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);     // explicitně – musí sedět s vysílačem
  radio.setChannel(TANK_RF_CHANNEL);
  radio.openReadingPipe(1, TANK_RF_ADDRESS);
  radio.startListening();

  Serial.print(F("radio.begin(): "));     Serial.println(ok ? F("OK") : F("FAIL"));
  Serial.print(F("isChipConnected(): ")); Serial.println(radio.isChipConnected() ? F("ANO") : F("NE - zkontroluj SPI/napajeni!"));

  if (!display.Initialize())
  {
    Serial.println(F("SSD1306 init failed"));

    for (;;)
    {
    }
  }

  display.ShowSplash();

  ha.Begin();   // WiFi + MQTT (samotné připojení dořeší ha.Loop())

  lastRx = millis();
}

void loop()
{
  if (radio.available())
  {
    TankReading reading;
    radio.read(&reading, sizeof(reading));
    lastRx = millis();
    signalLost = false;

    Serial.print(F("dist="));
    Serial.print(reading.distanceCm);
    Serial.print(F("cm  batt="));
    Serial.print(reading.batteryMv);
    Serial.print(F("mV  temp="));
    if (reading.temperatureC10 == TANK_TEMP_INVALID) Serial.print(F("ERR"));
    else                                             Serial.print(reading.temperatureC10 / 10.0f, 1);
    Serial.print(F("C  status="));
    Serial.println(reading.status);

    // Teplota/vlhkost jsou nezávislé na sonaru – ulož je vždy (i při NO_ECHO),
    // ať je THP obrazovka aktuální.
    lastTemperatureC10 = reading.temperatureC10;
    lastHumidityRh10   = reading.humidityRh10;
    if (screenIndex == 3) renderScreen();

    if (reading.status == TANK_NO_ECHO)
    {
      lastNoEcho = true;
      haveReading = true;
    }
    else
    {
      int volume = tank.GetActVolume(reading.distanceCm);
      const char* status = "";

      if (volume == 0)
      {
        status = "Sucho";
      }
      else if (volume > TANK_VOLUME)
      {
        status = "Nad kótou přelivu";
      }

      // Naplnění v procentech (oříznuté na 0–100) a odeslání do Home Assistantu.
      int percent = (int)((long)volume * 100 / TANK_VOLUME);
      if (percent < 0)   percent = 0;
      if (percent > 100) percent = 100;

      // Uložit poslední hodnoty pro překreslení obrazovek.
      lastNoEcho   = false;
      lastDistance = reading.distanceCm;
      lastVolume   = volume;
      lastPercent  = percent;
      lastStatus   = status;

      // První vzorek do grafu vlož hned po prvním měření.
      if (!haveReading) lastSample = millis() - SAMPLE_INTERVAL_MS;
      haveReading = true;

      ha.SetOnline(true);
      ha.PublishReading(reading.distanceCm, volume, reading.batteryMv, percent,
                        reading.temperatureC10, reading.humidityRh10);
    }

    if (screenIndex == 0) renderScreen();
  }

  // Vzorkování historie naplnění pro graf (plovoucí okno 24 h).
  if (haveReading && !lastNoEcho && millis() - lastSample >= SAMPLE_INTERVAL_MS)
  {
    lastSample = millis();
    history[histHead] = (int8_t)lastPercent;
    histHead = (histHead + 1) % HIST_SIZE;
    if (histCount < HIST_SIZE) histCount++;
    if (screenIndex == 1) renderScreen();
  }

  // Obsluha tlačítka – přepnutí na další obrazovku (s debounce).
  int b = digitalRead(BUTTON_PIN);
  if (b != buttonState && millis() - lastButtonChange > DEBOUNCE_MS)
  {
    lastButtonChange = millis();
    buttonState = b;
    if (b == LOW)   // stisk (interní pull-up)
    {
      screenIndex = (screenIndex + 1) % SCREEN_COUNT;
      renderScreen();
    }
  }

  if (!signalLost && millis() - lastRx > SIGNAL_TIMEOUT_MS)
  {
    signalLost = true;
    ha.SetOnline(false);   // ztráta signálu z čidla -> entity v HA zešednou
    if (screenIndex == 0) renderScreen();
  }

  ha.Loop();   // udržuje WiFi/MQTT spojení (neblokující)
}
