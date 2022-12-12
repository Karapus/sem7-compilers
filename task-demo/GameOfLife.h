#pragma once
#include "sim.h"

#ifndef SIM_X
#define SIM_X 10
#endif
#ifndef SIM_Y
#define SIM_Y 10
#endif

typedef enum { DEAD, ALIVE, CELL_T_END } cell_t;

typedef enum {
  ALIVE_MIN = 2,
  REPRODUCE_MIN = 3,
  OVERPOP_MIN = 4,
  NEIGHBOUR_COUNT_T_MAX = 8
} neighbour_count_t;

typedef unsigned x_t;
typedef unsigned y_t;

void seedGen(cell_t *Field);
void step(cell_t *Prev, cell_t *Next);
void fillImgBuffer(cell_t *Field);
