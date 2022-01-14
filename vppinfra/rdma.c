#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vppinfra/clib.h"
#include "vppinfra/vector.h"
#include "vppinfra/vector_sve.h"

#define CQE_FLAG_L4_OK			10
#define CQE_FLAG_L3_OK			9
#define CQE_FLAG_L2_OK			8
#define CQE_FLAG_IP_FRAG		7
#define CQE_FLAG_L4_HDR_TYPE(f)		(((f) >> 4) & 7)
#define CQE_FLAG_L3_HDR_TYPE_SHIFT	(2)
#define CQE_FLAG_L3_HDR_TYPE_MASK	(3 << CQE_FLAG_L3_HDR_TYPE_SHIFT)
#define CQE_FLAG_L3_HDR_TYPE(f)		(((f) & CQE_FLAG_L3_HDR_TYPE_MASK)  >> CQE_FLAG_L3_HDR_TYPE_SHIFT)
#define CQE_FLAG_L3_HDR_TYPE_IP4	1
#define CQE_FLAG_L3_HDR_TYPE_IP6	2
#define CQE_FLAG_IP_EXT_OPTS		1

/* CQE byte count (Striding RQ) */
#define CQE_BC_FILLER_MASK (1 << 31)
#define CQE_BC_CONSUMED_STRIDES_SHIFT (16)
#define CQE_BC_CONSUMED_STRIDES_MASK (0x3fff << CQE_BC_CONSUMED_STRIDES_SHIFT)
#define CQE_BC_BYTE_COUNT_MASK (0xffff)

static u32 gInput = 0xFFFFFFFF;

u64 clib_cpu_time_now (void)
{
  u64 vct;
  /* User access to cntvct_el0 is enabled in Linux kernel since 3.12. */
  asm volatile ("mrs %0, cntvct_el0":"=r" (vct));
  return vct;
}

static_always_inline int
slow_path_needed_sve (u32 buf_sz, int n_rx_packets,
					       u32 * bc)
{
#if defined CLIB_HAVE_VEC_SCALABLE
  i32 i = 0;
  boolxn m;
  u32xn bcv;
  i32 eno;
  u32xn thresh = u32xn_splat (buf_sz);
  scalable_vector_foreach2 (
    i, eno, m, n_rx_packets, 32, ({
      bcv = u32xn_load_unaligned (m, bc + i);
      if (boolxn_anytrue (m, u32xn_great_than (m, bcv, thresh)))
	return 1;
    }));
#endif

  return 0;
}

static_always_inline int
slow_path_needed_neon (u32 buf_sz, int n_rx_packets,
					       u32 * bc)
{
  u32x4 thresh4 = u32x4_splat (buf_sz);
  for (int i = 0; i < n_rx_packets; i += 4)
    if (!u32x4_is_all_zero (*(u32x4 *) (bc + i) > thresh4))
      return 1;

  return 0;
}

static_always_inline int
slow_path_needed_norm (u32 buf_sz, int n_rx_packets,
					       u32 * bc)
{
  while (n_rx_packets)
    {
      if (*bc > buf_sz)
	return 1;
      bc++;
      n_rx_packets--;
    }

  return 0;
}

static_always_inline int
validate_and_swap_bc_sve (u16 *cqe_flags, int n_rx_packets, u32 * bc)
{
  u16 mask = CQE_FLAG_L3_HDR_TYPE_MASK | CQE_FLAG_L3_OK;
  u16 match = CQE_FLAG_L3_HDR_TYPE_IP4 << CQE_FLAG_L3_HDR_TYPE_SHIFT;

  /* verify that all ip4 packets have l3_ok flag set and convert packet
     length from network to host byte order */
  int skip_ip4_cksum = 1;

#if defined CLIB_HAVE_VEC_SCALABLE
  i32 i = 0;
  boolxn m;
  i32 eno;
  u16xn maskv = u16xn_splat (mask);
  u16xn matchv = u16xn_splat (match);
  u16xn flagsv;
  u32xn bcv;
  scalable_vector_foreach2 (
    i, eno, m, n_rx_packets, 16, ({
      flagsv = u16xn_load_unaligned (m, cqe_flags + i);
      boolxn ne = u16xn_unequal (m, matchv, u16xn_and (m, flagsv, maskv));
      if (boolxn_anytrue (m, ne))
	{
	  skip_ip4_cksum = 0;
	  goto fast_ntoh;
	}
    }));
fast_ntoh:
  scalable_vector_foreach2 (i, eno, m, n_rx_packets, 32, ({
			      bcv = u32xn_load_unaligned (m, bc + i);
			      bcv = u32xn_byte_swap (m, bcv);
			      u32xn_store_unaligned (m, bcv, bc + i);
			    }));
#endif
  return skip_ip4_cksum;
}

static_always_inline int
validate_and_swap_bc_neon (u16 *cqe_flags, int n_rx_packets, u32 * bc)
{
  u16 mask = CQE_FLAG_L3_HDR_TYPE_MASK | CQE_FLAG_L3_OK;
  u16 match = CQE_FLAG_L3_HDR_TYPE_IP4 << CQE_FLAG_L3_HDR_TYPE_SHIFT;

  /* verify that all ip4 packets have l3_ok flag set and convert packet
     length from network to host byte order */
  int skip_ip4_cksum = 1;

#if defined CLIB_HAVE_VEC128
  u16x8 mask8 = u16x8_splat (mask);
  u16x8 match8 = u16x8_splat (match);
  u16x8 r = { };
  u16x8 cqe_flags8;

  for (int i = 0; i * 8 < n_rx_packets; i++)
  {
    cqe_flags8 = u16x8_load_unaligned (cqe_flags + i*8);
    r |= (cqe_flags8 & mask8) != match8;
  }

  if (!u16x8_is_all_zero (r))
    skip_ip4_cksum = 0;

  for (int i = 0; i < n_rx_packets; i += 4)
    *(u32x4 *) (bc + i) = u32x4_byte_swap (*(u32x4 *) (bc + i));
#endif
  return skip_ip4_cksum;
}

static_always_inline int
validate_and_swap_bc_norm (u16 *cqe_flags, int n_rx_packets, u32 * bc)
{
  u16 mask = CQE_FLAG_L3_HDR_TYPE_MASK | CQE_FLAG_L3_OK;
  u16 match = CQE_FLAG_L3_HDR_TYPE_IP4 << CQE_FLAG_L3_HDR_TYPE_SHIFT;

  /* verify that all ip4 packets have l3_ok flag set and convert packet
     length from network to host byte order */
  int skip_ip4_cksum = 1;

  for (int i = 0; i < n_rx_packets; i++)
    if ((cqe_flags[i] & mask) != match)
      skip_ip4_cksum = 0;

  for (int i = 0; i < n_rx_packets; i++)
    bc[i] = clib_net_to_host_u32 (bc[i]);
  return skip_ip4_cksum;
}

#define INITIAL 0
#define SVE_VERSION 1
#define NEON_VERSION 2
#define NORM_VERSION 3

static u64 stats[4][8];
int slowpath_benchmark (void)
{
    u64 cntsve = 0;
    u64 cntneon = 0;
    u64 dst64sve[1024];
    u64 dst64neon[1024];
    u64 dst64norm[1024];
    u32 src64[1024];
    u8 *dstsve = (u8 *) dst64sve;
    u8 *dstneon = (u8 *) dst64neon;
    u8 *dstnorm = (u8 *) dst64norm;
    u8 *src = (u8 *) src64;
    for (int i = 0; i < 1024; i++)
    {
        srand (time (0));
        src64[i] = ((u32) rand ()) & 0xFFFFFFF;
        dst64sve[i] = src64[0];
        dst64neon[i] = src64[0];
    }

    stats[INITIAL][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        slow_path_needed_sve (gInput, 1024, (u32 *) src64);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        slow_path_needed_neon (gInput, 1024, (u32 *) src64);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        slow_path_needed_norm (gInput, 1024, (u32 *) src64);
    }
    stats[NORM_VERSION][0] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\tNORM\n");
    for (int i = 0; i <= 0; i++)
    {
        printf ("%d\t%lu\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i],
                stats[NORM_VERSION][i] - stats[NEON_VERSION][i]);
    }

    if ((slow_path_needed_sve (gInput, 1024, (u32 *) src64) == slow_path_needed_neon (gInput, 1024, (u32 *) src64)) &&
        (slow_path_needed_norm (gInput, 1024, (u32 *) src64) == slow_path_needed_neon (gInput, 1024, (u32 *) src64)))
        printf ("Good!\n");
    else
        printf ("Failed!\n");
    return 0;
}

int validate_benchmark (void)
{
    u64 cntsve = 0;
    u64 cntneon = 0;
    u64 dst64sve[1024];
    u64 dst64neon[1024];
    u64 dst64norm[1024];
    u64 src64[1024];
    u8 *dstsve = (u8 *) dst64sve;
    u8 *dstneon = (u8 *) dst64neon;
    u8 *dstnorm = (u8 *) dst64norm;
    u8 *src = (u8 *) src64;
    u16 cqe_flags[1024];
    for (int i = 0; i < 1024; i++)
    {
        srand (time (0));
        src64[i] = (u64) rand ();
        dst64sve[i] = src64[0];
        dst64neon[i] = src64[0];
        cqe_flags[i] = gInput;
    }

    stats[INITIAL][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        validate_and_swap_bc_sve (cqe_flags, 1024, (u32 *) src64);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        validate_and_swap_bc_neon (cqe_flags, 1024, (u32 *) src64);
    }
    stats[NEON_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        validate_and_swap_bc_norm (cqe_flags, 1024, (u32 *) src64);
    }
    stats[NORM_VERSION][0] = clib_cpu_time_now ();

    printf ("Iter\tSVE\tNEON\tNORM\n");
    for (int i = 0; i <= 0; i++)
    {
        printf ("%d\t%lu\t%lu\t%lu\n", i, stats[SVE_VERSION][i] - stats[INITIAL][i],
                stats[NEON_VERSION][i] - stats[SVE_VERSION][i],
                stats[NORM_VERSION][i] - stats[NEON_VERSION][i]);
    }

    return 0;
}

int
main (int argc, char *argv[])
{
  gInput = (u32) atoi (argv[1]);
  printf ("slowpath_benchmark:\n");
  slowpath_benchmark ();
  printf ("validate_benchmark\n");
  validate_benchmark ();
  return 0;
}
