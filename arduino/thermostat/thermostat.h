class Thermostat
{
    private:
    //check if has remote control signal 
    //from upper control unit like raspberry
    boolean remoteControl;

    //if no remote control, then we should know
    //how the temperature is controlled
    //0 - off
    //1 - target temperature
    //2 - temperature curve 
    byte controlMode;

    //desired sensor temperature used in control mode 1
    //210 -> 21.0 C
    int TargetTemperature;

    //desired sensor temperature in 24 per-hour curve
    int TargetTemperatureGraph[24];

    //allowed span from target temperature
    int TemperatureSpan = 5; 

    public:
    //construct all defaults
    Thermostat();

    bool isRemoteControlled();
    byte getTargetTemperature();
    void setTargetTemperature(byte t);
    void setTargetTemperatureGraph(int[24]);
    void setTargetTemperatureGraph(string);

}