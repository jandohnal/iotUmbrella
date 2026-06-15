# Tank Level Meter

Bezdrátový měřič hladiny vody v nádrži. Skládá se ze dvou zařízení:

| Role | Deska | Úkol |
|------|-------|------|
| **sender** (vysílač) | Arduino Nano (ATmega328) | Ultrazvukem změří vzdálenost k hladině, odešle ji přes nRF24L01 a usne (napájeno baterií, důraz na spotřebu). |
| **receiver** (přijímač) | ESP32 (esp32dev) | Přijme surovou vzdálenost, dopočítá objem vody a zobrazí ho na OLED displeji. |

Sdílený formát rádiové zprávy je v [`lib/TankProtocol/TankProtocol.h`](lib/TankProtocol/TankProtocol.h) a linkuje se do obou prostředí.

Build / upload:

```sh
pio run -e sender   -t upload
pio run -e receiver -t upload
```

---

## Zapojení – Vysílač (Arduino Nano)

Komponenty: vodotěsný ultrazvukový senzor **AJ-SR04M / RCWL-1655** + rádiový modul **nRF24L01**.
Měření napětí baterie probíhá interně (proti referenci 1,1 V), **nepotřebuje žádný pin**.

### AJ-SR04M / RCWL-1655 (ultrazvukový senzor)

Vodotěsný senzor s odděleným měničem (JSN-SR04T kompatibilní), vhodný do nádrže.
V základním režimu **Mode 1** je Trig/Echo rozhraní pinově kompatibilní s HC-SR04, proto se používá knihovna NewPing beze změny.

| Pin senzoru | Arduino Nano | Poznámka |
|-------------|--------------|----------|
| VCC  | 5V   | napájení 5 V (modul snese 3,3–5,5 V, ale na 5 V má plný dosah) |
| TRIG | D9   | `TRIGGER_PIN` |
| ECHO | D4   | `ECHO_PIN` |
| GND  | GND  | |

> ⚠️ **ECHO nesmí být na D10.** D10 je hardwarový SS pin SPI; `radio.begin()` ho přepne na OUTPUT a echo by pak nešlo číst (`rawUs = 0`). Viz komentář v [`src/sender/main.cpp`](src/sender/main.cpp).
>
> 📏 **Mrtvá zóna ~20–25 cm.** Senzor neměří blíž než cca 25 cm, proto musí být umístěn dostatečně nad maximální hladinou. Tomu odpovídá `offset 58 cm` v konfiguraci nádrže ([`src/receiver/main.cpp`](src/receiver/main.cpp)). Max. dosah ≈ 4–4,5 m.
>
> 🔧 **Režim (Mode):** osazením rezistoru na pozici R19/R27 lze přepnout na UART (sériový) výstup. Tento projekt počítá s výchozím **Mode 1** (Trig/Echo) – nic neosazuj.

### nRF24L01 (rádio, hardwarové SPI)

| Pin modulu | Arduino Nano | Poznámka |
|------------|--------------|----------|
| VCC  | **3,3 V**     | modul je 3,3V – nepřipojovat na 5 V! |
| GND  | GND           | |
| CE   | D7            | `RF_CE_PIN` |
| CSN  | D8            | `RF_CSN_PIN` |
| SCK  | D13           | HW SPI SCK |
| MOSI | D11           | HW SPI MOSI |
| MISO | D12           | HW SPI MISO |
| IRQ  | – (nezapojeno)| přerušení se nepoužívá |

> 💡 Doporučení: mezi VCC a GND modulu nRF24L01 přidej **kondenzátor 10 µF** (blízko modulu). Stabilizuje napájení při vysílacích špičkách a řeší většinu problémů s `isChipConnected() = NE`.

---

## Zapojení – Přijímač (ESP32 / esp32dev)

Komponenty: rádiový modul **nRF24L01** + **OLED displej SSD1306 128×64 (I2C)**.

### nRF24L01 (rádio, VSPI)

| Pin modulu | ESP32 (GPIO) | Poznámka |
|------------|--------------|----------|
| VCC  | 3,3 V         | |
| GND  | GND           | |
| CE   | GPIO4         | `RF_CE_PIN` |
| CSN  | GPIO5         | `RF_CSN_PIN` |
| SCK  | GPIO18        | VSPI SCK |
| MISO | GPIO19        | VSPI MISO |
| MOSI | GPIO23        | VSPI MOSI |
| IRQ  | – (nezapojeno)| |

### OLED SSD1306 128×64 (hardwarové I2C)

| Pin displeje | ESP32 (GPIO) | Poznámka |
|--------------|--------------|----------|
| VCC | 3,3 V  | |
| GND | GND    | |
| SDA | GPIO21 | výchozí I2C SDA na ESP32 |
| SCL | GPIO22 | výchozí I2C SCL na ESP32 |

> Displej se inicializuje přes U8g2 s `U8X8_PIN_NONE` (bez reset pinu) – viz [`src/receiver/OledDisplay.cpp`](src/receiver/OledDisplay.cpp).

---

## Společná RF konfigurace

Aby spolu obě strany komunikovaly, musí sedět tyto hodnoty ([`TankProtocol.h`](lib/TankProtocol/TankProtocol.h)):

| Parametr | Hodnota |
|----------|---------|
| Kanál (`TANK_RF_CHANNEL`) | 76 |
| Adresa (`TANK_RF_ADDRESS`) | `0xF0F0F0F0E1` |
| Data rate | `RF24_1MBPS` |
| PA level | `RF24_PA_HIGH` |

---

## Schémata a referenční pinouty

Odkazy na pinouty a datasheety použitých desek a komponent:

- **Arduino Nano** – [oficiální pinout (Arduino Docs)](https://docs.arduino.cc/hardware/nano) · [pinout diagram PDF](https://content.arduino.cc/assets/Pinout-NANO_latest.pdf)
- **ESP32 DevKit (esp32dev)** – [Espressif pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html) · [Random Nerd Tutorials – ESP32 pinout reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- **nRF24L01** – [pinout obrázek (RF24 knihovna)](.pio/libdeps/sender/RF24/images/pinout.jpg) · [Nordic nRF24L01+ datasheet](https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf)
- **AJ-SR04M / RCWL-1655** – [datasheet (AJ-SR04M, dronebotworkshop)](https://dronebotworkshop.com/wp-content/uploads/2021/01/AJ-SR04M-Datasheet.pdf) · [popis režimů a zapojení](https://dronebotworkshop.com/waterproof-ultrasonic-sensor/)
- **OLED SSD1306 128×64 I2C** – [SSD1306 datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)

### Schémata zapojení projektu

| Vysílač (Arduino Nano) | Přijímač (ESP32) |
|------------------------|------------------|
| [![Schéma vysílače](docs/schema_sender.svg)](docs/schema_sender.svg) | [![Schéma přijímače](docs/schema_receiver.svg)](docs/schema_receiver.svg) |

Zdrojové soubory: [`docs/schema_sender.svg`](docs/schema_sender.svg) · [`docs/schema_receiver.svg`](docs/schema_receiver.svg)
