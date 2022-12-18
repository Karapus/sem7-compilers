#include <stdio.h>

int __print(int V) {
  printf("%d\n", V);
  return V;
}

int __qmark() {
  int Res;
  scanf("%d", &Res);
  return Res;
}
