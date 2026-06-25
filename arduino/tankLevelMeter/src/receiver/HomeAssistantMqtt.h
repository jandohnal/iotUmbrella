#pragma once
#include <WiFi.h>
#include <PubSubClient.h>

// =====================================================================
//  Odesílání naměřených dat do Home Assistantu přes MQTT.
//
//  - Připojí ESP32 na WiFi a k MQTT brokeru (běží na HA).
//  - Po připojení pošle "MQTT discovery" zprávy, takže se v HA samy
//    vytvoří entity (objem, výška hladiny, naplnění %, baterie) bez
//    ručního configuration.yaml.
//  - publishReading() pošle aktuální hodnoty, setOnline() řeší dostupnost.
//
//  Spojení se udržuje neblokujícím způsobem v loop() – příjem z rádia
//  tím není blokován.
// =====================================================================
class HomeAssistantMqtt
{
  public:
    HomeAssistantMqtt();

    // Spustí WiFi a nastaví MQTT (samotné připojení dořeší loop()).
    void Begin();

    // Volat v každém loop() – udržuje WiFi/MQTT spojení a obsluhuje klienta.
    void Loop();

    // Pošle aktuální měření do HA. Když není spojení, tiše se přeskočí.
    //   volumeL        – objem vody [l]
    //   distanceCm     – surová vzdálenost ze sonaru [cm]
    //   batteryMv      – napětí baterie vysílače [mV]
    //   percent        – naplnění nádrže [%]
    //   temperatureC10 – teplota z SHT40 [0,1 °C], TANK_TEMP_INVALID = vynechat
    //   humidityRh10   – vlhkost z SHT40 [0,1 %RH] (platí s teplotou)
    void PublishReading(int distanceCm, int volumeL, int batteryMv, int percent,
                        int temperatureC10, int humidityRh10);

    // Nastaví dostupnost (availability) v HA. Při ztrátě signálu z čidla
    // se dá nahlásit "offline" -> entity v HA zešednou jako nedostupné.
    void SetOnline(bool online);

    bool IsConnected();

    // --- Diagnostické info o WiFi (pro zobrazení na displeji) ---
    bool   IsWifiConnected();
    int    WifiRssi();        // síla signálu [dBm]
    String WifiIp();          // lokální IP adresa

  private:
    WiFiClient   wifiClient;
    PubSubClient mqtt;

    unsigned long lastReconnectAttempt;
    bool          discoverySent;
    bool          lastOnline;

    bool reconnect();
    void publishDiscovery();
    void publishOneSensor(const char* key, const char* name,
                          const char* unit, const char* deviceClass,
                          const char* icon);
};
