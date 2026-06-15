#include "WaterTank.h"

WaterTank::WaterTank(int h, int v, int o) {
  height      = h;
  volume      = v;
  offset      = o;
  volumePerCm = volume / height;
  totalHeight = height + offset;
}

int WaterTank::GetActVolume(int distance) {
  if (distance > totalHeight) return 0;
  if (distance < offset)      return volume + 1;
  
  return (totalHeight - distance) * volumePerCm;
}
