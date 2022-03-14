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

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//ultrasonic sensor
#define echoPin 10 // Echo Pin (OUTPUT pin in RB URF02)
#define trigPin 9 // Trigger Pin (INPUT pin in RB URF02)

//water tank parameters (in cm)
int height = 150;
int offset = 30;
int volume = 6000;

long distance; // calculated distance
int volumePerCm; // precalculated parameter of water tank
int timer = 10000; // let's measure every 10 seconds (some fancy time controll to be added later)

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  volumePerCm = volume / height;

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {

  distance = calcDistance();
  Serial.println(distance); // output to serial for raspberry
  displayData(distance);

  delay(timer); //Delay 50 ms
}

//use sensor to measure the distance
long calcDistance()
{
  long duration;

  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2); 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  return duration/58.2; //Calculate the distance (in cm) based on the speed of sound.
}

void displayData(int distance)
{
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print("L: ");
  display.print(distance);
  display.println(F(" cm"));

  if (distance > height + offset)
  {
    display.println("error 01");
  }
  else
  //calculate volume
  //height 150 cm -> 150 cm = 0 L, 0cm = 6000 L
  //1cm = 40 L
  display.print("V: ");
  display.print((distance-height-offset)*volumePerCm);
  display.println(F(" l"));
  display.display();
}