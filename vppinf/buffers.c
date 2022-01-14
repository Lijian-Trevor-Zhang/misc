#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vppinfra/clib.h"
#include "vppinfra/vector.h"
#include "vppinfra/vector_sve.h"

typedef u16 vlib_error_t;
#define VLIB_BUFFER_PRE_DATA_SIZE 128

u64 clib_cpu_time_now (void)
{
  u64 vct;
  /* User access to cntvct_el0 is enabled in Linux kernel since 3.12. */
  asm volatile ("mrs %0, cntvct_el0":"=r" (vct));
  return vct;
}
static u64 gbuffer_mem_start = 0xfeac000000L;
#define CLIB_LOG2_CACHE_LINE_BYTES 6
#define CLIB_CACHE_LINE_BYTES 64
#define CLIB_CACHE_LINE_ALIGN_MARK(mark)                                      \
  u8 mark[0] __attribute__ ((aligned (CLIB_CACHE_LINE_BYTES)))

/** VLIB buffer representation. */
typedef union
{
  struct
  {
    CLIB_CACHE_LINE_ALIGN_MARK (cacheline0);

    /** signed offset in data[], pre_data[] that we are currently
      * processing. If negative current header points into predata area.  */
    i16 current_data;

    /** Nbytes between current data and the end of this buffer.  */
    u16 current_length;

    /** buffer flags:
	<br> VLIB_BUFFER_FREE_LIST_INDEX_MASK: bits used to store free list index,
	<br> VLIB_BUFFER_IS_TRACED: trace this buffer.
	<br> VLIB_BUFFER_NEXT_PRESENT: this is a multi-chunk buffer.
	<br> VLIB_BUFFER_TOTAL_LENGTH_VALID: as it says
	<br> VLIB_BUFFER_EXT_HDR_VALID: buffer contains valid external buffer manager header,
	set to avoid adding it to a flow report
	<br> VLIB_BUFFER_FLAG_USER(n): user-defined bit N
     */
    u32 flags;

    /** Generic flow identifier */
    u32 flow_id;

    /** Reference count for this buffer. */
    volatile u8 ref_count;

    /** index of buffer pool this buffer belongs. */
    u8 buffer_pool_index;

    /** Error code for buffers to be enqueued to error handler.  */
    vlib_error_t error;

    /** Next buffer for this linked-list of buffers. Only valid if
      * VLIB_BUFFER_NEXT_PRESENT flag is set. */
    u32 next_buffer;

    /** The following fields can be in a union because once a packet enters
     * the punt path, it is no longer on a feature arc */
    union
    {
      /** Used by feature subgraph arcs to visit enabled feature nodes */
      u32 current_config_index;
      /* the reason the packet once punted */
      u32 punt_reason;
    };

    /** Opaque data used by sub-graphs for their own purposes. */
    u32 opaque[10];

    /** part of buffer metadata which is initialized on alloc ends here. */
      STRUCT_MARK (template_end);

    /** start of 2nd half (2nd cacheline on systems where cacheline size is 64) */
      CLIB_ALIGN_MARK (second_half, 64);

    /** Specifies trace buffer handle if VLIB_PACKET_IS_TRACED flag is
      * set. */
    u32 trace_handle;

    /** Only valid for first buffer in chain. Current length plus total length
      * given here give total number of bytes in buffer chain. */
    u32 total_length_not_including_first_buffer;

    /**< More opaque data, see ../vnet/vnet/buffer.h */
    u32 opaque2[14];

#if VLIB_BUFFER_TRACE_TRAJECTORY > 0
    /** trace trajectory data - we use a specific cacheline for that in the
     * buffer when it is compiled-in */
#define VLIB_BUFFER_TRACE_TRAJECTORY_MAX     31
#define VLIB_BUFFER_TRACE_TRAJECTORY_SZ	     64
#define VLIB_BUFFER_TRACE_TRAJECTORY_INIT(b) (b)->trajectory_nb = 0
    CLIB_ALIGN_MARK (trajectory, 64);
    u16 trajectory_nb;
    u16 trajectory_trace[VLIB_BUFFER_TRACE_TRAJECTORY_MAX];
#else /* VLIB_BUFFER_TRACE_TRAJECTORY */
#define VLIB_BUFFER_TRACE_TRAJECTORY_SZ 0
#define VLIB_BUFFER_TRACE_TRAJECTORY_INIT(b)
#endif /* VLIB_BUFFER_TRACE_TRAJECTORY */

    /** start of buffer headroom */
      CLIB_ALIGN_MARK (headroom, 64);

    /** Space for inserting data before buffer start.  Packet rewrite string
      * will be rewritten backwards and may extend back before
      * buffer->data[0].  Must come directly before packet data.  */
    u8 pre_data[VLIB_BUFFER_PRE_DATA_SIZE];

    /** Packet data */
    u8 data[];
  };
#ifdef CLIB_HAVE_VEC128
  u8x16 as_u8x16[4];
#endif
#ifdef CLIB_HAVE_VEC256
  u8x32 as_u8x32[2];
#endif
#ifdef CLIB_HAVE_VEC512
  u8x64 as_u8x64[1];
#endif
} vlib_buffer_t;

static_always_inline void
vlib_get_buffer_indices_with_offsetsve (void **b, u32 * bi,
				     uword count, i32 offset)
{
  uword buffer_mem_start = gbuffer_mem_start;
  i32 i;
  i32 eno;
  boolxn m;
  u64xn voff = u64xn_splat ((u64) (buffer_mem_start - offset));
  scalable_vector_foreach2 (
    i, eno, m, count, 64, ({
      u64xn vb = u64xn_load_unaligned (m, (u64 *) b + i);
      vb = u64xn_sub (m, vb, voff);
      vb = u64xn_shift_right_n (m, vb, CLIB_LOG2_CACHE_LINE_BYTES);
      u64xn_store_u32 (m, vb, bi + i);
    }));
  return;
}

always_inline u32
_vlib_get_buffer_index (void * vm, void *p)
{
  uword offset = pointer_to_uword (p) - gbuffer_mem_start;
  return offset >> CLIB_LOG2_CACHE_LINE_BYTES;
}
static_always_inline void
vlib_get_buffer_indices_with_offset (void **b, u32 * bi,
				     uword count, i32 offset)
{
  void * vm = NULL;
  while (count >= 4)
    {
      /* equivalent non-nector implementation */
      bi[0] = _vlib_get_buffer_index (vm, ((u8 *) b[0]) + offset);
      bi[1] = _vlib_get_buffer_index (vm, ((u8 *) b[1]) + offset);
      bi[2] = _vlib_get_buffer_index (vm, ((u8 *) b[2]) + offset);
      bi[3] = _vlib_get_buffer_index (vm, ((u8 *) b[3]) + offset);
      bi += 4;
      b += 4;
      count -= 4;
    }
  while (count)
    {
      bi[0] = _vlib_get_buffer_index (vm, ((u8 *) b[0]) + offset);
      bi += 1;
      b += 1;
      count -= 1;
    }
}
#define INITIAL 0
#define SVE_VERSION 1
#define NEON_VERSION 2
#define NORM_VERSION 3

static u64 stats[4][8];
static vlib_buffer_t b[2048];
static u32 idx[2048];
int memcpy_le_benchmark (void)
{
    u64 dst64sve[1024];
    u64 dst64neon[1024];
    u64 dst64norm[1024];
    u64 src64[1024];
    __clib_unused u8 *dstsve = (u8 *) dst64sve;
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
        vlib_get_buffer_indices_with_offsetsve ((void *) &b, idx, 1024, 4096);
        vlib_get_buffer_indices_with_offsetsve ((void *) &b, idx, 2047, 4096);
        vlib_get_buffer_indices_with_offsetsve ((void *) &b, idx, 511, 40960);
    }
    stats[SVE_VERSION][0] = clib_cpu_time_now ();
    for (int i = 0; i < 10240; i++)
    {
        vlib_get_buffer_indices_with_offset ((void *) &b, idx, 1024, 4096);
        vlib_get_buffer_indices_with_offset ((void *) &b, idx, 2047, 4096);
        vlib_get_buffer_indices_with_offset ((void *) &b, idx, 511, 40960);
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
