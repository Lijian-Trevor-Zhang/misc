#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vppinfra/clib.h"
#include "vppinfra/vector.h"
#include "vppinfra/vector_sve.h"

u64 clib_cpu_time_now (void)
{
  u64 vct;
  /* User access to cntvct_el0 is enabled in Linux kernel since 3.12. */
  asm volatile ("mrs %0, cntvct_el0":"=r" (vct));
  return vct;
}


static_always_inline void
clib_memset_u64sve (void *p, u64 val, uword count)
{
  u64 *ptr = p;
  i32 i, eno;
  u64xn v = u64xn_splat (val);
  boolxn m;
  scalable_vector_foreach2 (i, eno, m, count, 64,
			    ({ u64xn_store_unaligned (m, v, ptr + i); }));
  return;
}

static_always_inline void
clib_memset_u32sve (void *p, u32 val, uword count)
{
  u32 *ptr = p;
  i32 i, eno;
  u32xn v = u32xn_splat (val);
  boolxn m;
  scalable_vector_foreach2 (i, eno, m, count, 32,
			    ({ u32xn_store_unaligned (m, v, ptr + i); }));
  return;
}

static_always_inline void
clib_memset_u16sve (void *p, u16 val, uword count)
{
  u16 *ptr = p;
  i32 i, eno;
  u16xn v = u16xn_splat (val);
  boolxn m;
  scalable_vector_foreach2 (i, eno, m, count, 16,
			    ({ u16xn_store_unaligned (m, v, ptr + i); }));
  return;
}

static_always_inline void
clib_memset_u8sve (void *p, u8 val, uword count)
{
  u8 *ptr = p;
  i32 i, eno;
  u8xn v = u8xn_splat (val);
  boolxn m;
  scalable_vector_foreach2 (i, eno, m, count, 8,
			    ({ u8xn_store_unaligned (m, v, ptr + i); }));
  return;
}

static_always_inline void
clib_memset_u64 (void *p, u64 val, uword count)
{
  u64 *ptr = p;
  while (count >= 4)
    {
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = val;
      ptr += 4;
      count -= 4;
    }
  while (count--)
    ptr++[0] = val;
}

static_always_inline void
clib_memset_u32 (void *p, u32 val, uword count)
{
  u32 *ptr = p;
#if defined(CLIB_HAVE_VEC128) && defined(CLIB_HAVE_VEC128_UNALIGNED_LOAD_STORE)
  u32x4 v128 = u32x4_splat (val);
  while (count >= 4)
    {
      u32x4_store_unaligned (v128, ptr);
      ptr += 4;
      count -= 4;
    }
#else
  while (count >= 4)
    {
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = val;
      ptr += 4;
      count -= 4;
    }
#endif
  while (count--)
    ptr++[0] = val;
}

static_always_inline void
clib_memset_u16 (void *p, u16 val, uword count)
{
  u16 *ptr = p;
#if defined(CLIB_HAVE_VEC128) && defined(CLIB_HAVE_VEC128_UNALIGNED_LOAD_STORE)
  u16x8 v128 = u16x8_splat (val);
  while (count >= 8)
    {
      u16x8_store_unaligned (v128, ptr);
      ptr += 8;
      count -= 8;
    }
#else
  while (count >= 4)
    {
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = val;
      ptr += 4;
      count -= 4;
    }
#endif
  while (count--)
    ptr++[0] = val;
}

static_always_inline void
clib_memset_u8 (void *p, u8 val, uword count)
{
  u8 *ptr = p;
#if defined(CLIB_HAVE_VEC128) && defined(CLIB_HAVE_VEC128_UNALIGNED_LOAD_STORE)
  u8x16 v128 = u8x16_splat (val);
  while (count >= 16)
    {
      u8x16_store_unaligned (v128, ptr);
      ptr += 16;
      count -= 16;
    }
#else
  while (count >= 4)
    {
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = val;
      ptr += 4;
      count -= 4;
    }
#endif
  while (count--)
    ptr++[0] = val;
}
#define INITIAL 0
#define SVE_VERSION 1
#define NEON_VERSION 2
#define NORM_VERSION 3

static u64 stats[4][8];
int memcpy_le_benchmark (void)
{
    u64 dst64sve[1024];
    u64 dst64neon[1024];
    u64 dst64norm[1024];
    u64 src64[1024];
    u8 *dstsve = (u8 *) dst64sve;
    u8 *dstneon = (u8 *) dst64neon;
    __clib_unused u8 *dstnorm = (u8 *) dst64norm;
    __clib_unused u8 *src = (u8 *) src64;
    for (int i = 0; i < 1024; i++)
    {
        srand (time (0));
        src64[i] = (u64) rand ();
    }

    stats[INITIAL][0] = clib_cpu_time_now ();
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u64sve ((u64 *) dstsve, (u64) 0xdeafbeef, 511);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u32sve ((u64 *) dstsve+511, (u32) 0xdeafbeef, 65*2);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u16sve ((u64 *) dstsve+511+65, (u16) 0xdeafbeef, 127*4);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u8sve ((u64 *) dstsve+511+65+127, (u8) 0xdeafbeef, 92*8);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u64 ((u64 *) dstneon, (u64) 0xdeafbeef, 511);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u32 ((u64 *) dstneon+511, (u32) 0xdeafbeef, 65*2);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u16 ((u64 *) dstneon+511+65, (u16) 0xdeafbeef, 127*4);
    }
    for (int i = 0; i < 1024; i++)
    {
        clib_memset_u8 ((u64 *) dstneon+511+65+127, (u8) 0xdeafbeef, 92*8);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\n");
    for (int i = 0; i <= 0; i++)
    {
        printf ("%d\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i]);
    }

    if ((0 == memcmp (dstsve, dstneon, (511+65+127+92)*8)) &&
            (0 == memcmp (dstsve, dstneon, (511+65+127+92)*8)))
        printf ("Good!\n");
    else
        printf ("Failed!\n");
    return 0;
}

int
main (int argc, char *argv[])
{
  memcpy_le_benchmark ();
  return 0;
}
