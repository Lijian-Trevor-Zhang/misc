#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vppinfra/types.h"
#include "vppinfra/vector.h"

int main (int argc, char *argv[])
{
    time_t t;
    long i = 0;
    long n = strtol (argv[1], NULL, 10);

    u64x2 x;

    /* 初始化随机数发生器 */
    srand((unsigned) time(&t));

    x[0] = rand ();
    x[1] = rand ();

    while (i < n) {
        x[0] += u64x2_is_all_zero (x);
        x[1] += u64x2_is_all_zero (x);
        i++;
    }
    return x[0]+x[1];
}
