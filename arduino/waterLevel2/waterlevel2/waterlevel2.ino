#include <SPI.h>
#include <Wire.h>
#include <NewPing.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>

//relay
#define RELAY_PIN 3

//ultrasonic sensor
#define ECHO_PIN 10 // Echo Pin (OUTPUT pin in RB URF02)
#define TRIGGER_PIN 9 // Trigger Pin (INPUT pin in RB URF02)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

OledText128x64 MyDisplay = new OledText128x64();

//temperature and humidity sensor
Adafruit_AHTX0 aht;

// NewPing setup of pins and maximum distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, 400); 

WaterTank tank = new WaterTank(145, 6000, 58)

int timer = 1000; // let's measure every 1 seconds (some fancy time controll to be added later)

bool RelayState = false;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!MyDisplay.Initialize()) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  MyDisplay.ShowSplash();

  // display.display();
  // delay(2000); // Pause for 2 seconds

  if (! aht.begin(&Wire, 0x38)) {
    Serial.println("Could not find AHT? Check wiring");
    for(;;);
  }
  Serial.println("AHT10 or AHT20 found");

}

void loop() {

  //temperature and humidity sensor
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  //Serial.print("Temp: "); Serial.print(temp.temperature); Serial.println("  C");
  //Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

  //ultrasonic sensor
  distance = sonar.ping_cm();

  MyDisplay.Display(distance, 
    tank.GetActVolume(distance), 
    temp.temperature, 
    humidity.relative_humidity);

  // display.clearDisplay();
  // display.setTextSize(2);             // Normal 1:1 pixel scale
  // display.setTextColor(SSD1306_WHITE);        // Draw white text
  // display.setCursor(0,0);             // Start at top-left corner
  // display.print("l: "); display.print(distance); display.println(F(" cm"));
  // //display.println("hello");

  // display.print("V: "); display.print(tank.GetActVolume(distance)); display.println(F(" l"));

  
  // display.print("T:"); display.print(temp.temperature); display.println(" C");
  // display.print(humidity.relative_humidity); display.println(" %H");
  // display.display();

  // switch (RelayState)
  // {
  //   case true:
  //     Serial.print("turn on relay");
  //     digitalWrite(RELAY_PIN, HIGH);
  //     RelayState = false;
  //     break;
  //   case false:
  //     Serial.print("turn off relay");
  //     digitalWrite(RELAY_PIN, LOW);
  //     RelayState = true;
  //     break;
  // }

  delay(5000);
}

class OledText128x64
{
    private: 
        Adafruit_SSD1306 display;

    public:
    OledText128x64()
    {
        display(128, 64, &Wire, 4);
    }

        bool Initialize()
        {
            if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
                 return false;
            }
            return true;
        }

        void Display(int l, int v, int t, int h)
        {
          display.clearDisplay();
          display.setTextSize(2);             // Normal 1:1 pixel scale
          display.setTextColor(SSD1306_WHITE);        // Draw white text
          display.setCursor(0,0);             // Start at top-left corner
          string line1 = "L: ".concat(l).concat(" cm");
          string line2 = "V: ".concat(v).concat(" l");
          string line3 = "T: ".concat(t).concat(" C");
          string line4 = "H: ".concat(h).concat(" %");
          display.println(line1);
          display.println(line2);
          display.println(line3);
          display.println(line4);
        }

        void ShowSplash()
        {
          display.clearDisplay();
          display.display();
        }
}

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
    if (distance > totalHeight)
    {
      return -1;
    }
    //check if distance is smaller than offset of the sensor position
    if (distance < offset)
    {
      return -2;
    }
    
    return (totalHeight-distance)*volumePerCm;
  }
}

