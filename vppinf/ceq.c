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

static_always_inline uword
clib_count_equal_u64sve (u64 *data, uword max_count)
{
  uword count;
  u64 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  i32 i, eno;
  i32 len = (i32) max_count;
  u64xn s, splat = u64xn_splat (first);
  boolxn m, neq;
  scalable_vector_foreach2 (i, eno, m, len, 64, ({
			      s = u64xn_load_unaligned (m, data + i);
			      neq = u64xn_unequal (m, s, splat);
			      if (boolxn_anytrue (m, neq))
				{
				  count += u64xn_clz (m, neq);
				  return count;
				}
			      count += u64xn_clz (m, neq);
			    }));
  return count;
}

static_always_inline uword
clib_count_equal_u32sve (u32 *data, uword max_count)
{
  uword count;
  u32 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  i32 i, eno;
  i32 len = (i32) max_count;
  u32xn s, splat = u32xn_splat (first);
  boolxn m, neq;
  scalable_vector_foreach2 (i, eno, m, len, 32, ({
			      s = u32xn_load_unaligned (m, data + i);
			      neq = u32xn_unequal (m, s, splat);
			      if (boolxn_anytrue (m, neq))
				{
				  count += u32xn_clz (m, neq);
				  return count;
				}
			      count += u32xn_clz (m, neq);
			    }));
  return count;
}

static_always_inline uword
clib_count_equal_u16sve (u16 *data, uword max_count)
{
  uword count;
  u16 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  i32 i, eno;
  i32 len = (i32) max_count;
  u16xn s, splat = u16xn_splat (first);
  boolxn m, neq;
  scalable_vector_foreach2 (i, eno, m, len, 16, ({
			      s = u16xn_load_unaligned (m, data + i);
			      neq = u16xn_unequal (m, s, splat);
			      if (boolxn_anytrue (m, neq))
				{
				  count += u16xn_clz (m, neq);
				  return count;
				}
			      count += u16xn_clz (m, neq);
			    }));
  return count;
}

static_always_inline uword
clib_count_equal_u8sve (u8 *data, uword max_count)
{
  uword count;
  u8 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  i32 i, eno;
  i32 len = (i32) max_count;
  u8xn s, splat = u8xn_splat (first);
  boolxn m, neq;
  scalable_vector_foreach2 (i, eno, m, len, 8, ({
			      s = u8xn_load_unaligned (m, data + i);
			      neq = u8xn_unequal (m, s, splat);
			      if (boolxn_anytrue (m, neq))
				{
				  count += u8xn_clz (m, neq);
				  return count;
				}
			      count += u8xn_clz (m, neq);
			    }));
  return count;
}

static_always_inline uword
clib_count_equal_u64 (u64 *data, uword max_count)
{
  uword count;
  u64 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  count += 2;
  data += 2;
  while (count + 3 < max_count && ((data[0] ^ first) | (data[1] ^ first) |
				   (data[2] ^ first) | (data[3] ^ first)) == 0)
    {
      data += 4;
      count += 4;
    }
  while (count < max_count && (data[0] == first))
    {
      data += 1;
      count += 1;
    }
  return count;
}

static_always_inline uword
clib_count_equal_u32 (u32 *data, uword max_count)
{
  uword count;
  u32 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  u32x4 splat = u32x4_splat (first);
  while (count + 3 < max_count)
    {
      u64 bmp;
      bmp = u8x16_msb_mask ((u8x16) (u32x4_load_unaligned (data) == splat));
      if (bmp != pow2_mask (4 * 4))
	{
	  count += count_trailing_zeros (~bmp) / 4;
	  return count;
	}

      data += 4;
      count += 4;
    }
  while (count < max_count && (data[0] == first))
    {
      data += 1;
      count += 1;
    }
  return count;
}

static_always_inline uword
clib_count_equal_u16 (u16 *data, uword max_count)
{
  uword count;
  u16 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  u16x8 splat = u16x8_splat (first);
  while (count + 7 < max_count)
    {
      u64 bmp;
      bmp = u8x16_msb_mask ((u8x16) (u16x8_load_unaligned (data) == splat));
      if (bmp != 0xffff)
	{
	  count += count_trailing_zeros (~bmp) / 2;
	  return count;
	}

      data += 8;
      count += 8;
    }
  while (count < max_count && (data[0] == first))
    {
      data += 1;
      count += 1;
    }
  return count;
}

static_always_inline uword
clib_count_equal_u8 (u8 *data, uword max_count)
{
  uword count;
  u8 first;

  if (max_count <= 1)
    return max_count;
  if (data[0] != data[1])
    return 1;

  count = 0;
  first = data[0];

  u8x16 splat = u8x16_splat (first);
  while (count + 15 < max_count)
    {
      u64 bmp;
      bmp = u8x16_msb_mask ((u8x16) (u8x16_load_unaligned (data) == splat));
      if (bmp != 0xffff)
	return count + count_trailing_zeros (~bmp);

      data += 16;
      count += 16;
    }
  while (count < max_count && (data[0] == first))
    {
      data += 1;
      count += 1;
    }
  return count;
}

#define INITIAL 0
#define SVE_VERSION 1
#define NEON_VERSION 2
#define NORM_VERSION 3

static u64 stats[4][8];
int memcpy_le_benchmark (void)
{
    u64 cntsve = 0;
    u64 cntneon = 0;
    u64 dst64sve[1024];
    u64 dst64neon[1024];
    u64 dst64norm[1024];
    u64 src64[1024];
    u8 *dstsve = (u8 *) dst64sve;
    __clib_unused u8 *dstneon = (u8 *) dst64neon;
    __clib_unused u8 *dstnorm = (u8 *) dst64norm;
    __clib_unused u8 *src = (u8 *) src64;
    for (int i = 0; i < 1024; i++)
    {
        srand (time (0));
        src64[i] = (u64) rand ();
        dst64sve[i] = src64[0];
        dst64neon[i] = src64[0];
    }

    stats[INITIAL][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        cntsve += clib_count_equal_u64sve ((u64 *) dstsve, 1022);
        cntsve += clib_count_equal_u32sve ((u32 *) dstsve, 1022);
        cntsve += clib_count_equal_u16sve ((u16 *) dstsve, 1022);
        cntsve += clib_count_equal_u8sve ((u8 *) dstsve, 1022);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        cntneon += clib_count_equal_u64 ((u64 *) dstsve, 1022);
        cntneon += clib_count_equal_u32 ((u32 *) dstsve, 1022);
        cntneon += clib_count_equal_u16 ((u16 *) dstsve, 1022);
        cntneon += clib_count_equal_u8 ((u8 *) dstsve, 1022);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\n");
    for (int i = 0; i <= 0; i++)
    {
        printf ("%d\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i]);
    }

    if (cntsve == cntneon)
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
