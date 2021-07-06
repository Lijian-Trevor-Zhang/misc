#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vppinfra/types.h"
#include "vppinfra/vector.h"

int
main (int argc, char *argv[])
{
  u16x8 low = {10, 20, 30, 40, 50, 60, 70, 80};
  u16x8 hi = {16, 26, 36, 46, 56, 66, 76, 86};

  printf ("%s\n", u16x8_is_within_range (low, hi, 8)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 10)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 15)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 16)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 17)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 20)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 21)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 25)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 26)?"Yes":"No");
  printf ("%s\n", u16x8_is_within_range (low, hi, 27)?"Yes":"No");
  return 0;
}
