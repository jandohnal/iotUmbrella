#include "FanControl.h"

FanControl::FanControl(int hLim, int delay)
{
    _humidityLimit = hLim;
    _delay = delay * 60000;
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    IsOn = false;
}

void FanControl::PassHumidity(int h)
{
    if (IsOn && h < _humidityLimit && delayElapsed())
    {
        digitalWrite(RELAY_PIN, LOW);
        IsOn = false;
        _lastTimer = 0;
    }
    if (!IsOn && h > _humidityLimit)
    {
        digitalWrite(RELAY_PIN, HIGH);
        IsOn = true;
        _lastTimer = millis();
    }
}

bool FanControl::delayElapsed()
{
    if (_lastTimer != 0 && millis() - _lastTimer >= _delay)
    {
        return true;
    }
    return false;
}