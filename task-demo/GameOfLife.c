#include "GameOfLife.h"
#include <assert.h>

#define NNEIGH_ONE_SIDE 3

#define forEachCell(X, Y)                                                      \
  for (y_t Y = 0; Y < SIM_Y; ++Y)                                              \
    for (x_t X = 0; X < SIM_X; ++X)

#define forRow(XBegin, L, X, ACTION)                                           \
  do {                                                                         \
    assert(L < SIM_X);                                                         \
    for (x_t i = 0; i < L; ++i)                                                \
      do {                                                                     \
        x_t X = (XBegin + i) % L;                                              \
        do {                                                                   \
          ACTION;                                                              \
        } while (0);                                                           \
      } while (0);                                                             \
  } while (0);

static cell_t getNextState(cell_t *Field, x_t X, y_t Y);
static neighbour_count_t countNeighbours(cell_t *Field, x_t X, y_t Y);

static cell_t *getCell(cell_t *Field, x_t X, y_t Y) {
  assert(X < SIM_X);
  assert(Y < SIM_Y);
  return &Field[Y * SIM_X + X];
}

static cell_t getCellVal(cell_t *Field, x_t X, y_t Y) {
  assert(X < SIM_X);
  assert(Y < SIM_Y);
  return Field[Y * SIM_X + X];
}

static x_t getLeft(x_t X) { return (X ? X : SIM_X) - 1; }

static x_t getRight(x_t X) { return (X + 1) % SIM_X; }

static y_t getUp(y_t Y) { return (Y ? Y : SIM_Y) - 1; }

static y_t getDown(y_t Y) { return (Y + 1) % SIM_Y; }

static int cellToInt(cell_t Val) { return (Val == ALIVE) ? 1 : 0; }

const struct SIM_Color ALIVE_COLOR = {255, 255, 255, 255};
const struct SIM_Color DEAD_COLOR = {0, 0, 0, 255};

static struct SIM_Color getCellColor(cell_t Cell) {
  switch (Cell) {
  case ALIVE:
    return ALIVE_COLOR;
  default:
    return DEAD_COLOR;
  }
}

void seedGen(cell_t *Field) {
  forEachCell(X, Y) { *getCell(Field, X, Y) = simRand() % CELL_T_END; }
}

void step(cell_t *Prev, cell_t *Next) {
  forEachCell(X, Y) { *getCell(Next, X, Y) = getNextState(Prev, X, Y); }
}

void fillImgBuffer(cell_t *Field) {
  const unsigned XScale = IMG_X / SIM_X;
  const unsigned YScale = IMG_Y / SIM_Y;
  forEachCell(X, Y) {
    for (unsigned Yi = 0; Yi < YScale; ++Yi)
      for (unsigned Xi = 0; Xi < XScale; ++Xi)
        simSetPixel(X * XScale + Xi, Y * YScale + Yi,
                    getCellColor(getCellVal(Field, X, Y)));
  }
  simFlush();
}

cell_t getNextState(cell_t *Field, const x_t X, const y_t Y) {
  neighbour_count_t NNeighbours = countNeighbours(Field, X, Y);
  if (NNeighbours >= ALIVE_MIN && NNeighbours < OVERPOP_MIN) {
    if (NNeighbours >= REPRODUCE_MIN)
      return ALIVE;
    else
      return getCellVal(Field, X, Y);
  }
  return DEAD;
}

neighbour_count_t countNeighbours(cell_t *Field, const x_t X, const y_t Y) {
  int Res = 0;
  forRow(getLeft(X), NNEIGH_ONE_SIDE, XI,
         Res += cellToInt(getCellVal(Field, XI, getUp(Y)));) Res +=
      cellToInt(getCellVal(Field, getLeft(X), Y));
  Res += cellToInt(getCellVal(Field, getRight(X), Y));
  forRow(getLeft(X), NNEIGH_ONE_SIDE, XI,
         Res += cellToInt(getCellVal(Field, XI, getDown(Y))););
  return Res;
}
