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
bond_hash_to_port_sve (u32 * h, u32 n_left, u32 n_members,
		   int use_modulo_shortcut)
{
  u32 mask = n_members - 1;

#ifdef CLIB_HAVE_VEC_SCALABLE
  i32 i, eno;
  boolxn m;
  u32xn hv;
  u32xn maskv = u32xn_splat (mask);
  u32xn memberv = u32xn_splat (n_members);
  if (use_modulo_shortcut)
    {
      scalable_vector_foreach2 (i, eno, m, n_left, 32, ({
				  hv = u32xn_load_unaligned (m, h + i);
				  hv = u32xn_and (m, hv, maskv);
				  u32xn_store_unaligned (m, hv, h + i);
				}));
    }
  else
    {
      scalable_vector_foreach2 (i, eno, m, n_left, 32, ({
				  hv = u32xn_load_unaligned (m, h + i);
				  hv = u32xn_modulo (m, hv, memberv);
				  u32xn_store_unaligned (m, hv, h + i);
				}));
    }
#endif

  while (n_left > 4)
    {
      if (use_modulo_shortcut)
	{
	  h[0] &= mask;
	  h[1] &= mask;
	  h[2] &= mask;
	  h[3] &= mask;
	}
      else
	{
	  h[0] %= n_members;
	  h[1] %= n_members;
	  h[2] %= n_members;
	  h[3] %= n_members;
	}
      n_left -= 4;
      h += 4;
    }
  while (n_left)
    {
      if (use_modulo_shortcut)
	h[0] &= mask;
      else
	h[0] %= n_members;
      n_left -= 1;
      h += 1;
    }
}

static_always_inline void
bond_hash_to_port_norm (u32 * h, u32 n_left, u32 n_members,
		   int use_modulo_shortcut)
{
  u32 mask = n_members - 1;

  while (n_left > 4)
    {
      if (use_modulo_shortcut)
	{
	  h[0] &= mask;
	  h[1] &= mask;
	  h[2] &= mask;
	  h[3] &= mask;
	}
      else
	{
	  h[0] %= n_members;
	  h[1] %= n_members;
	  h[2] %= n_members;
	  h[3] %= n_members;
	}
      n_left -= 4;
      h += 4;
    }
  while (n_left)
    {
      if (use_modulo_shortcut)
	h[0] &= mask;
      else
	h[0] %= n_members;
      n_left -= 1;
      h += 1;
    }
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
    for (int i = 0; i < 102400; i++)
    {
        bond_hash_to_port_sve ((u32 *) src, 1025, 16, 1);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 102400; i++)
    {
        bond_hash_to_port_norm ((u32 *) src, 1025, 16, 1);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\n");
    for (int i = 0; i <= 0; i++)
    {
        printf ("%d\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i]);
    }

    return 0;
}

int
main (int argc, char *argv[])
{
  memcpy_le_benchmark ();
  return 0;
}
