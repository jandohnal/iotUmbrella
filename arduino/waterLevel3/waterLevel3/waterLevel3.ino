#include <SPI.h>
#include <Wire.h>
#include <NewPing.h>
#include <Adafruit_AHTX0.h>
#include "WaterTank.h"
#include "OledText128x64.h"
#include "FanControl.h"

//ultrasonic sensor
#define ECHO_PIN 10 // Echo Pin (OUTPUT pin in RB URF02)
#define TRIGGER_PIN 9 // Trigger Pin (INPUT pin in RB URF02)

//oled screen
OledText128x64 MyDisplay(4);

//fan control relay
FanControl fanControl(70, 1);

//temperature and humidity sensor
Adafruit_AHTX0 aht;

//sonar sensor using NewPing - digital pins and max distance
NewPing sonar(TRIGGER_PIN, ECHO_PIN, 400); 

//water tank (height, volume, offset
WaterTank tank(145, 6000, 58);

unsigned long lastTimer;
int timer = 10000; // let's measure every 10 seconds (some fancy time controll to be added later)

//distance for sonar sensor
int distance;

void setup() {
  Serial.begin(115200);
  lastTimer = 1;

  if(!MyDisplay.Initialize()) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  if (! aht.begin(&Wire, 0x38)) {
    Serial.println(F("Could not find AHT? Check wiring"));
    for(;;);
  }

}

void loop() {

  //measure every timer interval
  if(ItIsTime())
  {
    //temperature and humidity sensor
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    
    //ultrasonic sensor
    distance = sonar.ping_cm();

    //Serial.println(distance);
    //Serial.println(tank.GetActVolume(distance));
    //Serial.println(temp.temperature);
    //Serial.println(humidity.relative_humidity);

    //display data
    MyDisplay.Display(distance, tank.GetActVolume(distance), temp.temperature, humidity.relative_humidity);

    //pass humidity value to fan control to control the relay
    fanControl.PassHumidity(humidity.relative_humidity);
    lastTimer = millis();
  }
  
}

bool ItIsTime()
{
  if (millis() - lastTimer >= timer)
    {
        return true;
    }
    return false;
}