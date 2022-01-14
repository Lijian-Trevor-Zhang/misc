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

void *memmove(void *dest, const void *src, size_t n);

static_always_inline void
clib_memcpy_le_sve (u8 * dst, u8 * src, u8 len, u8 max_len)
{
  i32 eno = (i32) u8xn_max_elts ();
  boolxn m, all_m = u8xn_eltall_mask ();
  u8xn s0, s1, s2, s3;
  u8xn d0, d1, d2, d3;

  s0 = u8xn_load_unaligned (u8xn_elt_mask (eno * 0, max_len), src + eno * 0);
  d0 = u8xn_load_unaligned (u8xn_elt_mask (eno * 0, max_len), dst + eno * 0);
  m = u8xn_elt_mask (eno * 0, len);
  d0 = u8xn_sel (m, s0, d0);
  u8xn_store_unaligned (all_m, d0, dst + eno * 0);
  if (len <= eno)
    return;

  s1 = u8xn_load_unaligned (u8xn_elt_mask (eno * 1, max_len), src + eno * 1);
  d1 = u8xn_load_unaligned (u8xn_elt_mask (eno * 1, max_len), dst + eno * 1);
  m = u8xn_elt_mask (eno * 1, len);
  d1 = u8xn_sel (m, s1, d1);
  u8xn_store_unaligned (all_m, d1, dst + eno * 1);
  if ((max_len <= 32) || (len <= eno * 2))
    return;

  s2 = u8xn_load_unaligned (u8xn_elt_mask (eno * 2, max_len), src + eno * 2);
  d2 = u8xn_load_unaligned (u8xn_elt_mask (eno * 2, max_len), dst + eno * 2);
  m = u8xn_elt_mask (eno * 2, len);
  d2 = u8xn_sel (m, s2, d2);
  u8xn_store_unaligned (all_m, d2, dst + eno * 2);
  if (len <= eno * 3)
    return;

  s3 = u8xn_load_unaligned (u8xn_elt_mask (eno * 3, max_len), src + eno * 3);
  d3 = u8xn_load_unaligned (u8xn_elt_mask (eno * 3, max_len), dst + eno * 3);
  m = u8xn_elt_mask (eno * 3, len);
  d3 = u8xn_sel (m, s3, d3);
  u8xn_store_unaligned (all_m, d3, dst + eno * 3);
}

static_always_inline void
clib_memcpy_le_neon (u8 * dst, u8 * src, u8 len, u8 max_len)
{
  u8x16 s0, s1, s2, s3, d0, d1, d2, d3;
  u8x16 mask = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  u8x16 lv = u8x16_splat (len);
  u8x16 add = u8x16_splat (16);

  s0 = u8x16_load_unaligned (src);
  s1 = u8x16_load_unaligned (src + 16);
  s2 = u8x16_load_unaligned (src + 32);
  s3 = u8x16_load_unaligned (src + 48);
  d0 = u8x16_load_unaligned (dst);
  d1 = u8x16_load_unaligned (dst + 16);
  d2 = u8x16_load_unaligned (dst + 32);
  d3 = u8x16_load_unaligned (dst + 48);

  d0 = u8x16_blend (d0, s0, u8x16_is_greater (lv, mask));
  u8x16_store_unaligned (d0, dst);

  if (max_len <= 16)
    return;

  mask += add;
  d1 = u8x16_blend (d1, s1, u8x16_is_greater (lv, mask));
  u8x16_store_unaligned (d1, dst + 16);

  if (max_len <= 32)
    return;

  mask += add;
  d2 = u8x16_blend (d2, s2, u8x16_is_greater (lv, mask));
  u8x16_store_unaligned (d2, dst + 32);

  mask += add;
  d3 = u8x16_blend (d3, s3, u8x16_is_greater (lv, mask));
  u8x16_store_unaligned (d3, dst + 48);
}

static_always_inline void
clib_memcpy_le_norm (u8 * dst, u8 * src, u8 len, u8 max_len)
{
  memmove (dst, src, len);
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
    u8 *dstnorm = (u8 *) dst64norm;
    u8 *src = (u8 *) src64;
    for (int i = 0; i < 1024; i++)
    {
        srand (time (0));
        src64[i] = (u64) rand ();
    }

    stats[INITIAL][0] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_sve ((void *) dstsve, (void *) src, 32, 64);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_neon ((void *) dstneon, (void *) src, 32, 64);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_norm ((void *) dstnorm, (void *) src, 32, 64);
    }
    stats[NORM_VERSION][0] = clib_cpu_time_now ();

    stats[INITIAL][1] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_sve ((void *) dstsve+32, (void *) src+32, 47, 64);
    }
    stats[SVE_VERSION][1] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_neon ((void *) dstneon+32, (void *) src+32, 47, 64);
    }
    stats[NEON_VERSION][1] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_norm ((void *) dstnorm+32, (void *) src+32, 47, 64);
    }
    stats[NORM_VERSION][1] = clib_cpu_time_now ();

    stats[INITIAL][2] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_sve ((void *) dstsve+32+47, (void *) src+32+47, 48, 64);
    }
    stats[SVE_VERSION][2] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_neon ((void *) dstneon+32+47, (void *) src+32+47, 48, 64);
    }
    stats[NEON_VERSION][2] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_norm ((void *) dstnorm+32+47, (void *) src+32+47, 48, 64);
    }
    stats[NORM_VERSION][2] = clib_cpu_time_now ();

    stats[INITIAL][3] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_sve ((void *) dstsve+32+47, (void *) src+32+47+48, 16, 64);
    }
    stats[SVE_VERSION][3] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_neon ((void *) dstneon+32+47, (void *) src+32+47+48, 16, 64);
    }
    stats[NEON_VERSION][3] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_norm ((void *) dstnorm+32+47, (void *) src+32+47+48, 16, 64);
    }
    stats[NORM_VERSION][3] = clib_cpu_time_now ();

    stats[INITIAL][4] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_sve ((void *) dstsve+32+47, (void *) src+32+47+48+16, 32, 32);
    }
    stats[SVE_VERSION][4] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_neon ((void *) dstneon+32+47, (void *) src+32+47+48+16, 32, 32);
    }
    stats[NEON_VERSION][4] = clib_cpu_time_now ();
    for (int i = 0; i < 1024000; i++) {
        clib_memcpy_le_norm ((void *) dstnorm+32+47, (void *) src+32+47+48+16, 32, 32);
    }
    stats[NORM_VERSION][4] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\tNORM\n");
    for (int i = 0; i <= 4; i++)
    {
        printf ("%d\t%lu\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i],
                stats[NORM_VERSION][i] - stats[NEON_VERSION][i]);
    }

    if ((0 == memcmp (dstsve, dstneon, 32+47+48+16+32)) &&
            (0 == memcmp (dstnorm, dstneon, 32+47+48+16+32)))
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
