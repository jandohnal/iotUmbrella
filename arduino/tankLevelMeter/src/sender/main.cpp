// =====================================================================
//  VYSÍLAČ – Arduino Nano (ATmega328)
//  Ultrazvukový senzor + nRF24L01. Změří vzdálenost, odešle ji a usne.
//  Žádný výpočet objemu – ten běží až na přijímači (ESP32).
//  Důraz na minimální spotřebu (napájeno baterií).
// =====================================================================
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <NewPing.h>
#include <RF24.h>
#include <LowPower.h>
#include <TankProtocol.h>

// --- Piny ultrazvukového senzoru ---
// POZOR: Echo NESMÍ být na D10 – to je hardwarový SS pin SPI a radio.begin()
// ho přepne na OUTPUT, takže by echo nešlo číst (rawUs=0).
#define TRIGGER_PIN  9
#define ECHO_PIN     4
#define MAX_DISTANCE 400

// --- Piny nRF24L01 (mimo SPI 11/12/13 a piny senzoru 9/10) ---
#define RF_CE_PIN    7
#define RF_CSN_PIN   8

// --- Teplotní/vlhkostní senzor SHT40 (I2C) ---
// I2C je na ATmega328 pevně na A4 = SDA, A5 = SCL. Modul SHT40 obvykle nese
// vlastní pull-upy; pokud ne, přidej 4k7–10k z SDA i SCL na 3,3 V.
// Napájení VCC ber z 3,3 V railu (senzor je 1,08–3,6 V).

// Ladicí režim: bez spánku, vysílá každou 1 s, vše vypisuje na sériák.
// Po odladění přepni zpět na 0 (kvůli spotřebě).
#define SENDER_DEBUG 0

// Kolik 8s spánkových cyklů mezi měřeními (8 × 8 s ≈ 64 s).
static const uint8_t SLEEP_CYCLES = 8;

// --- Filtrace měření ---
// Kolik pingů v jedné dávce a jaká max. odchylka [cm] od mediánu se ještě
// počítá do průměru. Hodnoty dál od mediánu se zahodí jako šum/odraz od stěny.
static const uint8_t MEAS_SAMPLES = 5;
static const uint8_t TOLERANCE_CM = 5;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
RF24 radio(RF_CE_PIN, RF_CSN_PIN);
SensirionI2cSht4x sht4x;

// Změří napětí Vcc proti interní referenci 1,1 V (bez externích součástek).
static long readVccMv()
{
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2);                          // ustálení reference

  ADCSRA |= _BV(ADSC);               // start převodu

  while (bit_is_set(ADCSRA, ADSC))   // počkej na dokončení
  {
  }

  uint16_t result = ADCL;
  result |= ADCH << 8;

  if (result == 0)
  {
    return 0;
  }

  return 1125300L / result;          // 1,1 V × 1023 × 1000
}

// Přečte teplotu [0,1 °C] i vlhkost [0,1 %RH] z SHT40 jedním měřením.
// Při chybě (žádná odpověď / CRC) nastaví teplotu na TANK_TEMP_INVALID
// a vlhkost na 0 – odběratel poznává neplatnost podle teploty.
static void readSht40(int16_t &temperatureC10, uint16_t &humidityRh10)
{
  float temperatureC = 0.0f;
  float humidityRh   = 0.0f;

  int16_t error = sht4x.measureHighPrecision(temperatureC, humidityRh);

  if (error)
  {
    temperatureC10 = TANK_TEMP_INVALID;
    humidityRh10   = 0;
    return;
  }

  if (humidityRh < 0.0f)   humidityRh = 0.0f;     // SHT40 může vrátit lehce mimo
  if (humidityRh > 100.0f) humidityRh = 100.0f;   // rozsah 0–100 %RH

  temperatureC10 = (int16_t)lroundf(temperatureC * 10.0f);
  humidityRh10   = (uint16_t)lroundf(humidityRh * 10.0f);
}

// Změří vzdálenost: udělá MEAS_SAMPLES pingů, vyřadí odlehlé hodnoty
// (vzdálené od mediánu víc než TOLERANCE_CM) a ze zbytku spočte průměr.
// Vrací cm, nebo 0 když žádný ping nevrátil platný odraz.
static unsigned int measureDistanceCm()
{
  unsigned int samples[MEAS_SAMPLES];
  uint8_t n = 0;

  // 1) Nasbírej platné odrazy (NO_ECHO = 0 zahoď rovnou).
  for (uint8_t i = 0; i < MEAS_SAMPLES; i++)
  {
    unsigned int cm = sonar.convert_cm(sonar.ping());

    if (cm > 0)
    {
      samples[n++] = cm;
    }

    delay(30);   // odstup mezi pingy, ať dozní ozvěny
  }

  if (n == 0)
  {
    return 0;   // žádný odraz
  }

  // 2) Seřaď (malé pole -> bublinka stačí) a vezmi medián jako referenci.
  for (uint8_t i = 0; i + 1 < n; i++)
  {
    for (uint8_t j = 0; j + 1 < n - i; j++)
    {
      if (samples[j] > samples[j + 1])
      {
        unsigned int t = samples[j];

        samples[j] = samples[j + 1];
        samples[j + 1] = t;
      }
    }
  }

  unsigned int median = samples[n / 2];

  // 3) Zprůměruj jen hodnoty blízko mediánu.
  unsigned long sum = 0;
  uint8_t cnt = 0;

  for (uint8_t i = 0; i < n; i++)
  {
    unsigned int diff = (samples[i] > median)
      ? samples[i] - median
      : median - samples[i];

    if (diff <= TOLERANCE_CM)
    {
      sum += samples[i];
      cnt++;
    }
  }

  return cnt
    ? (unsigned int)(sum / cnt)
    : median;   // medián filtrem projde vždy
}

void setup()
{
#if SENDER_DEBUG
  Serial.begin(115200);
  Serial.println(F("\n--- SENDER start ---"));
#endif

  Wire.begin();
  sht4x.begin(Wire, SHT40_I2C_ADDR_44);

  bool ok = radio.begin();

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);     // explicitně – musí sedět s přijímačem
  radio.setChannel(TANK_RF_CHANNEL);
  radio.openWritingPipe(TANK_RF_ADDRESS);
  radio.stopListening();

#if SENDER_DEBUG
  Serial.print(F("radio.begin(): "));     Serial.println(ok ? F("OK") : F("FAIL"));
  Serial.print(F("isChipConnected(): ")); Serial.println(radio.isChipConnected() ? F("ANO") : F("NE - zkontroluj SPI/napajeni!"));
#else
  radio.powerDown();                 // rádio spí, probudí se až před odesláním
#endif
}

void loop()
{
  TankReading reading;
#if SENDER_DEBUG
  unsigned int rawUs   = sonar.ping();          // surový čas letu [µs]
  unsigned int distance = sonar.convert_cm(rawUs);
#else
  // Průměr z pingů po vyřazení odlehlých hodnot (viz measureDistanceCm).
  unsigned int distance = measureDistanceCm();
#endif

  reading.distanceCm = distance;
  reading.status     = (distance == 0) ? TANK_NO_ECHO : TANK_OK;
  reading.batteryMv  = (uint16_t)readVccMv();
  readSht40(reading.temperatureC10, reading.humidityRh10);

  radio.powerUp();
  delay(5);                          // čas na ustálení oscilátoru rádia
  bool sent = radio.write(&reading, sizeof(reading));

#if SENDER_DEBUG
  Serial.print(F("rawUs=")); Serial.print(rawUs);
  Serial.print(F("  dist="));  Serial.print(reading.distanceCm);
  Serial.print(F("cm  temp="));
  if (reading.temperatureC10 == TANK_TEMP_INVALID)
  {
    Serial.print(F("ERR"));
  }
  else
  {
    Serial.print(reading.temperatureC10 / 10.0f, 1);
    Serial.print(F("C  hum="));
    Serial.print(reading.humidityRh10 / 10.0f, 1);
    Serial.print(F("%"));
  }
  Serial.print(F("  ack=")); Serial.println(sent ? F("OK (prijimac potvrdil)") : F("FAIL (zadny ACK)"));
  delay(1000);                       // bez spánku, ať se dá sledovat
#else
  radio.powerDown();

  for (uint8_t i = 0; i < SLEEP_CYCLES; i++)   // hluboký spánek kvůli baterii
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
#endif
}
