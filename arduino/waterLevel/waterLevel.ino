/**************************************************************************
calculate water level using ultrasonic sensor
parameters 
- height - distance between bottom and top of the water tank
- offset - distance between sensor and top of water tank
- volume - volume of tank in liters
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//ultrasonic sensor
#define ECHO_PIN 10 // Echo Pin (OUTPUT pin in RB URF02)
#define TRIGGER_PIN 9 // Trigger Pin (INPUT pin in RB URF02)

//water tank parameters (in cm)
int height = 145;
int offset = 58;
int volume = 6000;

long distance; // calculated distance
int volumePerCm; // precalculated parameter of water tank
int timer = 1000; // let's measure every 1 seconds (some fancy time controll to be added later)

// NewPing setup of pins and maximum distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, 400); 

void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  volumePerCm = volume / height;

  // Show Adafruit splash screen. (it's nice :)
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {

  distance = sonar.ping_cm();
  Serial.println(distance); // output to serial for raspberry
  displayData(distance);

  delay(timer); //Delay
}

void displayData(int distance)
{
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print("l: ");
  display.print(distance);
  display.println(F(" cm"));

  //check if distance is not bigger than height of the water tank and offset of the sensor position
  if (distance > height + offset)
  {
    display.println("error 01");
    display.display();
  }
  //check if distance is smaller than offset of the sensor position
  else if (distance < offset)
  {
    display.println("error 02");
    display.display();
  }
  else
  {
    //calculate volume
    display.print("V: ");
    display.print((height+offset-distance)*volumePerCm);
    display.println(F(" l"));
    display.display();
  }
}
