#include "GameOfLife.h"

int main() {
  cell_t Fields[SIM_X * SIM_Y][2];
  seedGen(Fields[0]);
  while (simIsRunning()) {
    step(Fields[0], Fields[1]);
    fillImgBuffer(Fields[0]);
    step(Fields[1], Fields[0]);
    fillImgBuffer(Fields[1]);
  }
}
