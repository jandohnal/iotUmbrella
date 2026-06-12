#pragma once

// Výpočet objemu vody v nádrži ze vzdálenosti naměřené sonarem.
//   height       – výška vodního sloupce při plné nádrži [cm]
//   volume       – objem při plné nádrži [l]
//   offset       – vzdálenost senzoru od hladiny plné nádrže [cm]
class WaterTank {
  private:
    int height;
    int volume;
    int offset;
    int volumePerCm;
    int totalHeight;

  public:
    WaterTank(int h, int v, int o);

    // Vrací objem [l] pro danou vzdálenost [cm].
    //   -1 = vzdálenost větší než dno nádrže (nesmysl / chyba)
    //   -2 = vzdálenost menší než offset (nad plnou hladinou)
    int GetActVolume(int distance);
};
