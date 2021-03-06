#include "mfunc.h"

#include <pmmintrin.h>

#define ENABLE_SLOT0 1
#define ENABLE_SLOT1 1

#if (!ENABLE_SLOT0) && (!ENABLE_SLOT1)
#error At least one slot must by enabled!
#endif

typedef union {
    __m128d m128d;
    unsigned long long int ints[2];
} int_union;    


static int check_slot(int slot, int *iterations, int_union *test, int *in_progress,
        __m128d *cx, __m128d *cy, __m128d *zr, __m128d *zi,
        PIXEL_SOURCE next_pixel, PIXEL_OUTPUT output_pixel, BATON *baton)
{
    union {
        __m128d m128d;
        double doubles[2];
    } pixel_x, pixel_y, zero_x, zero_y;

    if (*iterations > 0 && !test->ints[slot])
        return 1;
    
    if (*in_progress & (1 << slot))
    {
        pixel_x.m128d = *zr;
        pixel_y.m128d = *zi;
        output_pixel(slot, *iterations, pixel_x.doubles[slot], pixel_y.doubles[slot], baton);
    }
    else
    {
        *in_progress |= (1 << slot);
    }

    if (next_pixel(slot, iterations, &zero_x.doubles[slot], &zero_y.doubles[slot], &pixel_x.doubles[slot], &pixel_y.doubles[slot], baton))
    {
        if (slot == 0)
        {
            *cx = _mm_move_sd(*cx, pixel_x.m128d);
            *cy = _mm_move_sd(*cy, pixel_y.m128d);
            *zr = _mm_move_sd(*zr, zero_x.m128d);
            *zi = _mm_move_sd(*zi, zero_y.m128d);
        }
        else
        {
            *cx = _mm_move_sd(pixel_x.m128d, *cx);
            *cy = _mm_move_sd(pixel_y.m128d, *cy);
            *zr = _mm_move_sd(zero_x.m128d, *zr);
            *zi = _mm_move_sd(zero_y.m128d, *zi);
        }
    }
    else
    {
        *in_progress &= ~(1 << slot);
    }

    if (*in_progress == 0)
        return 0;
    
    return 1;
}


void mfunc_simd(ALLOCATE_SLOTS allocate_slots, PIXEL_SOURCE next_pixel, PIXEL_OUTPUT output_pixel, BATON *baton)
{
    int i0 = 0;
    int i1 = 0;
    int in_progress = 0;

    __m128d cx;
    __m128d cy;
    __m128d zr;
    __m128d zi;
    __m128d zr2;
    __m128d zi2;
    __m128d t, t2;
    __m128d boundary;

    int_union test;
    
    int countdown_from;
    int countdown;

    allocate_slots(ENABLE_SLOT1 ? 2 : 1, baton);
    
    boundary = _mm_set1_pd(2.0*2.0);
    cx = _mm_set1_pd(0.0);
    cy = _mm_set1_pd(0.0);
    zr = _mm_set1_pd(0.0);
    zi = _mm_set1_pd(0.0);

    while (1)
    {
        /* Check if it's time to output the first pixel and/or start a new one. */
        if (ENABLE_SLOT0 && !check_slot(0, &i0, &test, &in_progress, &cx, &cy, &zr, &zi, next_pixel, output_pixel, baton))
            break;

        /* Check if it's time to output the second pixel and/or start a new one. */
        if (ENABLE_SLOT1 && !check_slot(1, &i1, &test, &in_progress, &cx, &cy, &zr, &zi, next_pixel, output_pixel, baton))
            break;

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

        countdown_from = MIN(i0, i1);
        if (countdown_from <= 0)
            countdown_from = 1;
        countdown = countdown_from;

        while (1)
        {
            /* Do some work on the current pixel. */
            zr2 = _mm_mul_pd(zr, zr);
            zi2 = _mm_mul_pd(zi, zi);
            t = _mm_mul_pd(zr, zi);
            zr = _mm_sub_pd(zr2, zi2);
            zr = _mm_add_pd(zr, cx);
            zi = _mm_add_pd(t, t);
            zi = _mm_add_pd(zi, cy);

            countdown--;

            /* Check against the boundary. */
            t2 = _mm_add_pd(zr2, zi2);
            t2 = _mm_cmpgt_pd(t2, boundary);

            if (countdown == 0 || _mm_movemask_pd(t2))
                break;
        }
        test.m128d = t2;
        i0 -= (countdown_from - countdown);
        i1 -= (countdown_from - countdown);
    }
}
