//water tank holding water
//cubic shape
//ultrasonic sensor is 
class WaterTank
{
  private:
  int height; //tank height
  int volume; //total volume (in liters)
  int offset; //distance of the sensor above top of the tank
  int volumePerCm; //calculation of how many liters equat to one centimeter of height of the tank
  int totalHeight; // height of the tank + distance of the sensor above top of the tank

  public:
  //constructor
  //height of the tank
  //total volume of the tank
  //offset of the sensor
  WaterTank(int h, int v, int o);

    //calculate actual volume of water in the tank
    //distance - measured sensor value - distance between sensor and water
    //-1 if distance is bigger than totalHeight
    //-2 if distance is less than offset
  int GetActVolume(int distance);

};