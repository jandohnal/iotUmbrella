#include <SPI.h>
#include <Wire.h>
#include <NewPing.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RELAY_PIN 3
#define ECHO_PIN 10
#define TRIGGER_PIN 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4
#define SCREEN_ADDRESS 0x3C

class WaterTank
{
  private:
  int height;
  int volume;
  int offset;
  int volumePerCm;
  int totalHeight;

  public:
  WaterTank(int h, int v, int o)
  {
    height = h;
    volume = v;
    offset = o;
    volumePerCm = volume / height;
    totalHeight = height + offset;
  }

  int GetActVolume(int distance)
  {
    if (distance > totalHeight) return -1;
    if (distance < offset) return -2;
    return (totalHeight - distance) * volumePerCm;
  }
};

class OledText128x64
{
  private:
    Adafruit_SSD1306 display;

  public:
  OledText128x64() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

  bool Initialize()
  {
    return display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  }

  void Display(int l, int v)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("L: " + String(l) + " cm");
    display.println("V: " + String(v) + " l");
    display.display();
  }

  void ShowSplash()
  {
    display.clearDisplay();
    display.display();
  }
};

NewPing sonar(TRIGGER_PIN, ECHO_PIN, 400);
WaterTank tank(145, 6000, 58);
OledText128x64 MyDisplay;

bool RelayState = false;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);

  if (!MyDisplay.Initialize()) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  MyDisplay.ShowSplash();
}

void loop() {
  int distance = sonar.ping_cm();

  unsigned int uS = sonar.ping();
  int distanceT = sonar.convert_cm(uS);
  
  Serial.print("Raw us: ");
  Serial.print(uS);
  Serial.print("  Distance cm: ");
  Serial.println(distanceT);

  MyDisplay.Display(distance, tank.GetActVolume(distance));
  delay(500);
}
