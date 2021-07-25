#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vppinfra/types.h"
#include "vppinfra/vector.h"

int main (int argc, char *argv[])
{
    u16x8 a = {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17};
    u16x8 b = {0xFFFF,0x20,0x2,0x1,0x1,0xFFF0,0x0,0x17};
    u16x8 c = u16x8_sub_saturate (a, b);
    u16x8 d = u16x8_add_saturate (a, b);
    return 0;
}
