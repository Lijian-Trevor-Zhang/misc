#include <stdio.h>
#include "vppinfra/clib.h"
#include "vppinfra/vector_sve.h"

int __u8xn_max_elts (void)
{
    printf ("u8xn_max_elts: %d\n", u8xn_max_elts ());
    return 16 == u8xn_max_elts () ? 0 : 1;
}

int __u16xn_max_elts (void)
{
    printf ("u16xn_max_elts: %d\n", u16xn_max_elts ());
    return 8 == u16xn_max_elts () ? 0 : 1;
}

int __u32xn_max_elts (void)
{
    printf ("u32xn_max_elts: %d\n", u32xn_max_elts ());
    return 4 == u32xn_max_elts () ? 0 : 1;
}

int __u64xn_max_elts (void)
{
    printf ("u64xn_max_elts: %d\n", u64xn_max_elts ());
    return 2 == u64xn_max_elts () ? 0 : 1;
}

int __u16xn_compact_store (void)
{
    u16 v[8], x[8];
    boolxn m;
    for (int i = 0; i < 8; i++)
    {
        v[i] = i;
        x[i] = 100 + i;
    }
    u16 a[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    u16 b[8] = {1, 0, 0, 1, 0, 1, 1, 0};
    u16xn av = u16xn_load_unaligned (u16xn_eltall_mask (), a);
    u16xn bv = u16xn_load_unaligned (u16xn_eltall_mask (), b);
    m = u16xn_equal (u16xn_eltall_mask (), av, bv);
    u16xn vv = u16xn_load_unaligned (u16xn_eltall_mask (), v);
    u16xn_compact_store (m, vv, x);
    for (int i = 0; i < 8; i++)
    {
        printf ("%4X ", x[i]);
    }
    printf ("\n");
    if ((0 == x[0]) &&
            (3 == x[1]) &&
            (5 == x[2]) &&
            (6 == x[3]) &&
            (104 == x[4]) &&
            (105 == x[5]) &&
            (106 == x[6]) &&
            (107 == x[7]))
        return 0;
    else
        return 1;
    return 0;
}

int __u8xn_memcmp (void)
{
    u8 a[] = {0xf1, 0x00, 0x0a, 0xae, 0xac, 0xfe, 0x00, 0x0a,
        0x0b, 0x28, 0x29, 0x29, 0x0a, 0xae, 0xac, 0xfe,
        0xfb, 0xb8, 0x79, 0x89, 0x8a, 0x5e, 0xcc, 0xfe,
        0x4b, 0x28, 0x29, 0x29, 0x0a, 0xae, 0xac, 0xfe,
    };
    u8 b[] = {0xf1, 0x00, 0x0a, 0xae, 0xac, 0xfe, 0x00, 0x0a,
        0x0b, 0x28, 0x29, 0x29, 0x0a, 0xae, 0xac, 0xfe,
        0xfb, 0xb8, 0x79, 0x89, 0x8a, 0x5e, 0xcc, 0xfe,
        0x4b, 0x28, 0x29, 0x29, 0x0a, 0xae, 0xac, 0xfe,
    };

    if ((1 != u8xn_memcmp (a, b, 7)) ||
            (1 != u8xn_memcmp (a, b, 13)) ||
            (1 != u8xn_memcmp (a, b, 16)) ||
            (1 != u8xn_memcmp (a, b, 17)) ||
            (1 != u8xn_memcmp (a, b, 23)) ||
            (1 != u8xn_memcmp (a, b, 31)))
        return 1;

    a[11] = 0x03;

    if (1 != u8xn_memcmp (a, b, 7))
        return 1;
    if (1 != u8xn_memcmp (a, b, 8))
        return 1;
    if (1 != u8xn_memcmp (a, b, 9))
        return 1;

    if ((1 == u8xn_memcmp (a, b, 13)) ||
            (1 == u8xn_memcmp (a, b, 14)) ||
            (1 == u8xn_memcmp (a, b, 16)) ||
            (1 == u8xn_memcmp (a, b, 17)) ||
            (1 == u8xn_memcmp (a, b, 23)) ||
            (1 == u8xn_memcmp (a, b, 31)))
        return 1;

    return 0;
}

int __u64xn_store_u8 (void)
{
    u64 x[2] = {0x12345678, 0x8765432a};
    u8 y[16] = {0};
    u64xn xv = u64xn_load_unaligned (u64xn_eltall_mask (), x);
    u64xn_store_u8 (u16xn_eltall_mask (), xv, y);
    for (int i = 0; i < 16; i++)
    {
        printf ("%2X ", y[i]);
    }
    printf ("\n");
    return 0;
}

int __cla_clz (void)
{
    u16 a[8] = {0, 1, 1, 1, 1, 1, 1, 1};
    u16 b[8] = {1, 0, 0, 1, 0, 1, 1, 0};
    u16xn av = u16xn_load_unaligned (u16xn_eltall_mask (), a);
    u16xn bv = u16xn_load_unaligned (u16xn_eltall_mask (), b);
    boolxn m = u16xn_equal (u16xn_eltall_mask (), av, bv);
        // m = 0, 0, 0, 1, 0, 1, 1, 0
    if ((0 != u16xn_cla (u16xn_eltall_mask (), m)) ||
        (3 != u16xn_clz (u16xn_eltall_mask (), m)))
        return 1;
    
    u16 x[8] = {1, 0, 1, 1, 1, 1, 1, 1};
    u16 y[8] = {1, 0, 1, 0, 0, 1, 1, 0};
    u16xn xv = u16xn_load_unaligned (u16xn_eltall_mask (), x);
    u16xn yv = u16xn_load_unaligned (u16xn_eltall_mask (), y);
    boolxn n = u16xn_equal (u16xn_eltall_mask (), xv, yv);
        // n = 1, 1, 1, 0, 0, 1, 1, 0
    if ((3 != u16xn_cla (u16xn_eltall_mask (), n)) ||
        (0 != u16xn_clz (u16xn_eltall_mask (), n)))
        return 1;
    return 0;
}

int main (int argc, char *argv[])
{
	u8 a[1024];
	u8 b[1024];
	u8xn_memcmp (a, b, 64);
	printf ("Hello World!\n");
    __u8xn_max_elts ();
    if (__u8xn_max_elts () ||
            __u16xn_max_elts () ||
            __u32xn_max_elts () ||
            __u64xn_max_elts () ||
            __u8xn_memcmp () ||
            __u64xn_store_u8 () ||
            __cla_clz () ||
            __u16xn_compact_store ())
    {
        printf ("Failure...\n");
    } else {
        printf ("Success...\n");
    }

	return 0;
}
