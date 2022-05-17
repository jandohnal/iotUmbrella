#include <Arduino.h>

//relay
#define RELAY_PIN 3

class FanControl
{
    private:
    int _humidityLimit;
    long _delay;
    unsigned long _lastTimer;

    public:
    bool IsOn;

    public:
    //constructor
    //hLim - humidity limit when the relay should be turned on/off
    //thresh - how long should we keep the fan turned on in minutes
    FanControl(int hLim, int delay);

    void PassHumidity(int h);

    private:
    bool delayElapsed();
};