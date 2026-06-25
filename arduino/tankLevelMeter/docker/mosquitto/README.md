# Mosquitto MQTT broker (Docker)

MQTT broker pro Home Assistant běžící v Dockeru (HA Container – nemá add-ony).
Na tento broker se připojuje ESP32 přijímač i Home Assistant.

## 1. Vytvoř uživatele a heslo

Mosquitto 2.x nedovolí anonymní přístup, je potřeba soubor `passwd` s heslem.
Vytvoříš ho přímo přes image (nahraď `tank` a `supersecret` svými hodnotami):

```sh
cd docker/mosquitto
docker run --rm -v "$(pwd)/config:/mosquitto/config" eclipse-mosquitto:2 \
  mosquitto_passwd -b -c /mosquitto/config/passwd tank supersecret
```

Vznikne `config/passwd` (heslo je hashované). Pro přidání dalšího uživatele
spusť totéž bez `-c`.

> `config/passwd` necommituj – je v `.gitignore`.

## 2. Spusť broker

```sh
docker compose up -d
docker logs -f mosquitto      # kontrola, že naběhl bez chyb
```

## 3. Připoj Home Assistant

V HA: **Nastavení → Zařízení a služby → Přidat integraci → MQTT**.

- **Broker:** IP/hostname stroje, kde běží Mosquitto (pokud běží HA ve stejném
  Docker compose, můžeš použít název služby `mosquitto`).
- **Port:** `1883`
- **Uživatel / Heslo:** stejné jako v kroku 1 (`tank` / `supersecret`).

## 4. Nastav stejné údaje na ESP32

V [`../../src/receiver/Secrets.h`](../../src/receiver/Secrets.h):

```c
#define MQTT_HOST      "192.168.1.10"   // IP stroje s Mosquittem
#define MQTT_PORT      1883
#define MQTT_USER      "tank"
#define MQTT_PASSWORD  "supersecret"
```

Po nahrání firmwaru se v HA pod zařízením **Tank Level Meter** samy objeví
entity (objem, vzdálenost, naplnění, baterie) díky MQTT discovery.

## Ověření provozu

```sh
docker exec -it mosquitto mosquitto_sub -h localhost -u tank -P supersecret -t '#' -v
```

Vypisuje veškerý provoz – po nahrání ESP32 tu uvidíš zprávy na
`tanklevelmeter/state`.
