#pragma once
#include <stdint.h>

// =====================================================================
//  Sdílený kontrakt mezi vysílačem (Arduino Nano) a přijímačem (ESP32).
//  Tento soubor MUSÍ být identický pro obě zařízení – proto leží v lib/
//  a PlatformIO ho automaticky přilinkuje do obou prostředí.
// =====================================================================

// Stavové kódy měření
enum TankStatus : uint8_t
{
  TANK_OK      = 0,
  TANK_NO_ECHO = 1,   // sonar nevrátil žádný odraz (mimo dosah / chyba)
};

// Hodnota teploty signalizující neúspěšné čtení ze senzoru SHT40.
static const int16_t TANK_TEMP_INVALID = INT16_MIN;

// Payload posílaný přes nRF24L01.
// __attribute__((packed)) zaručuje stejnou velikost (9 B) na AVR i ESP32 –
// jinak by se kvůli zarovnání lišila a komunikace by se rozbila.
struct __attribute__((packed)) TankReading
{
  uint16_t distanceCm;      // surová vzdálenost ze sonaru [cm]
  uint16_t batteryMv;       // napětí baterie vysílače [mV]
  int16_t  temperatureC10;  // teplota z SHT40 [0,1 °C], TANK_TEMP_INVALID = chyba
  uint16_t humidityRh10;    // vlhkost z SHT40 [0,1 %RH]; platí jen když je
                            //   temperatureC10 platná (čte se jedním měřením)
  uint8_t  status;          // viz TankStatus
};

// Společná RF konfigurace – musí sedět na obou stranách.
static const uint8_t  TANK_RF_CHANNEL = 76;
static const uint64_t TANK_RF_ADDRESS = 0xF0F0F0F0E1ULL;
