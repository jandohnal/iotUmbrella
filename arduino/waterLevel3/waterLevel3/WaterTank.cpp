#include "WaterTank.h"


WaterTank::WaterTank(int h, int v, int o)
{
    height = h;
    volume = v;
    offset = o;
    volumePerCm = volume / height;
    totalHeight = height + offset;
}

int WaterTank::GetActVolume(int distance)
{
    //if measured distance is higher than totalHeight return error
    if (distance > totalHeight)
    {
        return -1;
    }
    //if measured distance is less then offset return error
    if (distance < offset)
    {
        return -2;
    }

    return (totalHeight-distance)*volumePerCm;
}
