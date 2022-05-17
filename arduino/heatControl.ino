// this should be idea of control system of heat unit
//heat unit signal relay
//grundfos 220V relay
//HDO signal
//HDO manual input
//desired room temperature
//temperature span

#include <SPI.h>
#include <Wire.h>

#include "thermostat/thermostat.h"

void setup()
{
    Serial.begin(9600);
    Thermostat thermostat = new Thermostat();
}

void loop()
{
    if (Serial.available())
    {
        String command = Serial.readString();
    }
}