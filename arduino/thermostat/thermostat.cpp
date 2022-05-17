#include "thermostat.h"
using namespace std;

Thermostat::Thermostat()
{
    remoteControl = false;
    controlMode = 1;
    TargetTemperature = 210;
    TemperatureSpan = 5;
}