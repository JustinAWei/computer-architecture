#include <stdio.h>
#include <stdlib.h>
#include <math.h>

long long n,k,d,x,y;

int main(int argc, char* argv[]) {
    for (int i=1; i<argc; i++) {
      n = atoll(argv[i]);

      // k is the first value such that k^2 is greater than n
      // this determines the 'layer' that n resides on
      k = sqrt(n)+1;

      // distance of n from the edge of the layer, k^2-1
      d  = k*k - 1 - n;

      // equation for coordinates
      x = fmin(k-1, d);
      y = fmin(2*(k-1)-d,k-1);

      // swap x,y depending on k parity
      if(k&1) {x^=y; y^=x; x^=y;}

      printf("%lld (%lld,%lld)\n",n,x,y);
    }
    return 0;
}
