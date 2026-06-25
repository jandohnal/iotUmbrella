#pragma once

// =====================================================================
//  Přihlašovací údaje pro WiFi a MQTT (Home Assistant).
//
//  Tento soubor je ŠABLONA. Zkopíruj ho na "Secrets.h" a vyplň své
//  hodnoty:
//      cp src/receiver/Secrets.example.h src/receiver/Secrets.h
//
//  Secrets.h je v .gitignore, takže se necommitne (hesla nepatří do gitu).
// =====================================================================

// --- WiFi ---
#define WIFI_SSID      "tvoje-wifi"
#define WIFI_PASSWORD  "tvoje-heslo"

// --- MQTT broker (běží na Home Assistantu) ---
#define MQTT_HOST      "192.168.1.10"   // IP nebo hostname brokera
#define MQTT_PORT      1883
#define MQTT_USER      "mqtt-user"       // prázdné "" pokud broker bez autentizace
#define MQTT_PASSWORD  "mqtt-heslo"
