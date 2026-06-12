// =====================================================================
//  VYSÍLAČ – Arduino Nano (ATmega328)
//  Ultrazvukový senzor + nRF24L01. Změří vzdálenost, odešle ji a usne.
//  Žádný výpočet objemu – ten běží až na přijímači (ESP32).
//  Důraz na minimální spotřebu (napájeno baterií).
// =====================================================================
#include <Arduino.h>
#include <SPI.h>
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

// Ladicí režim: bez spánku, vysílá každou 1 s, vše vypisuje na sériák.
// Po odladění přepni zpět na 0 (kvůli spotřebě).
#define SENDER_DEBUG 0

// Kolik 8s spánkových cyklů mezi měřeními (8 × 8 s ≈ 64 s).
static const uint8_t SLEEP_CYCLES = 8;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Změří napětí Vcc proti interní referenci 1,1 V (bez externích součástek).
static long readVccMv() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);                          // ustálení reference
  ADCSRA |= _BV(ADSC);               // start převodu
  while (bit_is_set(ADCSRA, ADSC));  // počkej na dokončení
  uint16_t result = ADCL;
  result |= ADCH << 8;
  if (result == 0) return 0;
  return 1125300L / result;          // 1,1 V × 1023 × 1000
}

void setup() {
#if SENDER_DEBUG
  Serial.begin(115200);
  Serial.println(F("\n--- SENDER start ---"));
#endif

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

void loop() {
  TankReading reading;
#if SENDER_DEBUG
  unsigned int rawUs   = sonar.ping();          // surový čas letu [µs]
  unsigned int distance = sonar.convert_cm(rawUs);
#else
  // Medián z 5 pingů – odfiltruje náhodné výpadky odrazu.
  unsigned int distance = sonar.convert_cm(sonar.ping_median(5));
#endif

  reading.distanceCm = distance;
  reading.status     = (distance == 0) ? TANK_NO_ECHO : TANK_OK;
  reading.batteryMv  = (uint16_t)readVccMv();

  radio.powerUp();
  delay(5);                          // čas na ustálení oscilátoru rádia
  bool sent = radio.write(&reading, sizeof(reading));

#if SENDER_DEBUG
  Serial.print(F("rawUs=")); Serial.print(rawUs);
  Serial.print(F("  dist="));  Serial.print(reading.distanceCm);
  Serial.print(F("cm  ack=")); Serial.println(sent ? F("OK (prijimac potvrdil)") : F("FAIL (zadny ACK)"));
  delay(1000);                       // bez spánku, ať se dá sledovat
#else
  radio.powerDown();
  for (uint8_t i = 0; i < SLEEP_CYCLES; i++) {   // hluboký spánek kvůli baterii
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
#endif
}
