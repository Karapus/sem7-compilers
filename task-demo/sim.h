#pragma once
#ifndef IMG_X
#define IMG_X 800
#endif
#ifndef IMG_Y
#define IMG_Y 800
#endif

struct SIM_Color {
  unsigned char R;
  unsigned char G;
  unsigned char B;
  unsigned char A;
};

void simSetPixel(int x, int y, struct SIM_Color Color);
void simFlush();
int simRand();
int simIsRunning();
