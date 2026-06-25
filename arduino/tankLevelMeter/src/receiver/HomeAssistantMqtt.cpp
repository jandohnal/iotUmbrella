#include "HomeAssistantMqtt.h"
#include "Secrets.h"
#include <Arduino.h>
#include <TankProtocol.h>   // TANK_TEMP_INVALID

// =====================================================================
//  MQTT topiky
//    - state:        sem se publikuje JSON se všemi hodnotami
//    - availability: online/offline (+ LWT, ať HA pozná spadlé zařízení)
//    - discovery:    homeassistant/sensor/<node>/<key>/config (retained)
//  NODE_ID musí být unikátní v rámci HA.
// =====================================================================
static const char* NODE_ID      = "tanklevelmeter";
static const char* STATE_TOPIC  = "tanklevelmeter/state";
static const char* AVTY_TOPIC   = "tanklevelmeter/status";

// Po jak dlouhé pauze zkusit znovu připojit MQTT (neblokující backoff).
static const unsigned long RECONNECT_INTERVAL_MS = 5000;

HomeAssistantMqtt::HomeAssistantMqtt()
  : mqtt(wifiClient),
    lastReconnectAttempt(0),
    discoverySent(false),
    lastOnline(false)
{
}

void HomeAssistantMqtt::Begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // Diagnostický sken: vypíše všechny sítě 2,4 GHz, které ESP32 vidí.
  // Pokud tu cílová síť není, je jen na 5 GHz / jiný název / mimo dosah.
  Serial.println(F("WiFi: skenuji site (2.4GHz)..."));
  WiFi.disconnect(true);   // ukončit případné staré spojení před skenem
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.print(F("WiFi: nalezeno siti = "));
  Serial.println(n);       // -1=bezi, -2=selhalo, 0=nic v dosahu
  for (int i = 0; i < n; i++) {
    Serial.print(F("  "));
    Serial.print(WiFi.SSID(i));
    Serial.print(F("  RSSI="));
    Serial.print(WiFi.RSSI(i));
    Serial.print(F("  ch="));
    Serial.println(WiFi.channel(i));
  }
  WiFi.scanDelete();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);   // HA discovery zprávy se do výchozích 256 B nevejdou

  Serial.print(F("WiFi: pripojuji k "));
  Serial.println(WIFI_SSID);
}

bool HomeAssistantMqtt::IsConnected() {
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

bool HomeAssistantMqtt::IsWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int HomeAssistantMqtt::WifiRssi() {
  return WiFi.RSSI();
}

String HomeAssistantMqtt::WifiIp() {
  return WiFi.localIP().toString();
}

void HomeAssistantMqtt::Loop() {
  // Při změně stavu WiFi vypiš info na sériovou linku (pomáhá při ladění:
  // ověříš, že je ESP32 na síti, a zjistíš jeho IP/MAC).
  static bool wifiWasConnected = false;
  bool wifiNow = (WiFi.status() == WL_CONNECTED);
  if (wifiNow != wifiWasConnected) {
    wifiWasConnected = wifiNow;
    if (wifiNow) {
      Serial.print(F("WiFi: pripojeno, IP="));
      Serial.print(WiFi.localIP());
      Serial.print(F("  MAC="));
      Serial.print(WiFi.macAddress());
      Serial.print(F("  RSSI="));
      Serial.println(WiFi.RSSI());
    } else {
      Serial.println(F("WiFi: odpojeno"));
    }
  }

  // Dokud nejsme připojení, vypisuj každé 2 s stavový kód WiFi – z něj
  // poznáš příčinu (špatné heslo / nenalezené SSID / 5GHz apod.).
  if (!wifiNow) {
    static unsigned long lastWifiLog = 0;
    if (millis() - lastWifiLog >= 2000) {
      lastWifiLog = millis();
      Serial.print(F("WiFi: cekam, status="));
      switch (WiFi.status()) {
        case WL_IDLE_STATUS:    Serial.println(F("0 IDLE")); break;
        case WL_NO_SSID_AVAIL:  Serial.println(F("1 NO_SSID (sit nevidi - jmeno? 5GHz?)")); break;
        case WL_CONNECT_FAILED: Serial.println(F("4 CONNECT_FAILED (spatne heslo?)")); break;
        case WL_CONNECTION_LOST:Serial.println(F("5 CONNECTION_LOST")); break;
        case WL_DISCONNECTED:   Serial.println(F("6 DISCONNECTED (pripojuji...)")); break;
        default:                Serial.println(WiFi.status()); break;
      }
    }
    return;   // bez WiFi nemá smysl řešit MQTT
  }

  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
      lastReconnectAttempt = now;
      reconnect();
    }
    return;
  }

  mqtt.loop();
}

bool HomeAssistantMqtt::reconnect() {
  Serial.print(F("MQTT: pripojuji k "));
  Serial.print(MQTT_HOST);
  Serial.print(F(" ... "));

  // LWT (last will): když spojení spadne, broker za nás pošle "offline".
  bool ok = mqtt.connect(NODE_ID, MQTT_USER, MQTT_PASSWORD,
                         AVTY_TOPIC, 0, true, "offline");

  if (!ok) {
    Serial.print(F("FAIL, rc="));
    Serial.println(mqtt.state());
    return false;
  }

  Serial.println(F("OK"));

  // Po (re)připojení vždy znovu vyhlásit discovery a dostupnost.
  publishDiscovery();
  mqtt.publish(AVTY_TOPIC, lastOnline ? "online" : "offline", true);
  return true;
}

void HomeAssistantMqtt::SetOnline(bool online) {
  if (online == lastOnline) return;
  lastOnline = online;

  if (mqtt.connected()) {
    mqtt.publish(AVTY_TOPIC, online ? "online" : "offline", true);
  }
}

// Pošle jednu discovery zprávu pro jeden senzor. HA z ní vytvoří entitu.
void HomeAssistantMqtt::publishOneSensor(const char* key, const char* name,
                                         const char* unit, const char* deviceClass,
                                         const char* icon) {
  char topic[96];
  snprintf(topic, sizeof(topic),
           "homeassistant/sensor/%s/%s/config", NODE_ID, key);

  // Volitelná pole (dev_cla / ic) přidáme jen když mají hodnotu.
  char devCla[48] = "";
  if (deviceClass && deviceClass[0]) {
    snprintf(devCla, sizeof(devCla), "\"dev_cla\":\"%s\",", deviceClass);
  }
  char iconFld[48] = "";
  if (icon && icon[0]) {
    snprintf(iconFld, sizeof(iconFld), "\"ic\":\"%s\",", icon);
  }

  char payload[512];
  snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"%s\","
      "\"uniq_id\":\"%s_%s\","
      "\"stat_t\":\"%s\","
      "\"avty_t\":\"%s\","
      "\"unit_of_meas\":\"%s\","
      "%s"                                  // dev_cla (volitelné)
      "\"stat_cla\":\"measurement\","
      "%s"                                  // ic (volitelné)
      "\"val_tpl\":\"{{ value_json.%s }}\","
      "\"dev\":{\"ids\":[\"%s\"],\"name\":\"Tank Level Meter\","
              "\"mdl\":\"ESP32 + nRF24\",\"mf\":\"iotUmbrella\"}"
    "}",
    name, NODE_ID, key, STATE_TOPIC, AVTY_TOPIC, unit,
    devCla, iconFld, key, NODE_ID);

  mqtt.publish(topic, payload, true);   // retained, ať HA config drží po restartu
}

void HomeAssistantMqtt::publishDiscovery() {
  publishOneSensor("volume",   "Objem vody",      "L",  "",        "mdi:water");
  publishOneSensor("distance", "Vzdalenost",      "cm", "distance", "mdi:arrow-expand-vertical");
  publishOneSensor("percent",  "Naplneni",        "%",  "",        "mdi:gauge");
  publishOneSensor("battery",  "Baterie vysilace","V",  "voltage", "");
  publishOneSensor("temperature","Teplota",       "°C", "temperature", "");
  publishOneSensor("humidity",  "Vlhkost",        "%",  "humidity",    "");
  discoverySent = true;
}

void HomeAssistantMqtt::PublishReading(int distanceCm, int volumeL,
                                       int batteryMv, int percent,
                                       int temperatureC10, int humidityRh10) {
  Serial.print(F("MQTT publish: mqtt.connected()="));
  Serial.println(mqtt.connected());
  if (!mqtt.connected()) return;

  // Teplotu + vlhkost přidáme jen když je čtení platné (senzor mohl selhat) –
  // obojí pochází z jednoho měření, takže stačí kontrola teploty. Záporné
  // hodnoty a případ -0,x °C řešíme samostatným znaménkem, ať se neztratí.
  char thpFields[48] = "";
  if (temperatureC10 != TANK_TEMP_INVALID) {
    int absC10 = temperatureC10 < 0 ? -temperatureC10 : temperatureC10;
    snprintf(thpFields, sizeof(thpFields),
             ",\"temperature\":%s%d.%d,\"humidity\":%d.%d",
             temperatureC10 < 0 ? "-" : "", absC10 / 10, absC10 % 10,
             humidityRh10 / 10, humidityRh10 % 10);
  }

  char payload[200];
  snprintf(payload, sizeof(payload),
    "{\"distance\":%d,\"volume\":%d,\"percent\":%d,\"battery\":%d.%03d%s}",
    distanceCm, volumeL, percent, batteryMv / 1000, batteryMv % 1000, thpFields);

  mqtt.publish(STATE_TOPIC, payload);
}
