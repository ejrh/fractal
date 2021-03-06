#include "mfunc.h"

#include <xmmintrin.h>

#define ENABLE_SLOT0 1
#define ENABLE_SLOT1 1
#define ENABLE_SLOT2 1
#define ENABLE_SLOT3 1

#if (!ENABLE_SLOT0) && (!ENABLE_SLOT1) && (!ENABLE_SLOT2) && (!ENABLE_SLOT3)
#error At least one slot must by enabled!
#endif

typedef union {
    __m128 m128;
    unsigned long int ints[4];
} int_union;

typedef union {
    __m128 m128;
    float floats[4];
} float_union;


static int check_slot(int slot, int *iterations, int_union *test, int *in_progress,
        float_union *cx, float_union *cy, float_union *zr, float_union *zi,
        PIXEL_SOURCE next_pixel, PIXEL_OUTPUT output_pixel, BATON *baton)
{
    float_union pixel_x, pixel_y;
    
    double zx, zy;
    double px, py;

    if (*iterations > 0 && !test->ints[slot])
        return 1;
    
    if (*in_progress & (1 << slot))
    {
        pixel_x.m128 = zr->m128;
        pixel_y.m128 = zi->m128;
        output_pixel(slot, *iterations, pixel_x.floats[slot], pixel_y.floats[slot], baton);
    }
    else
    {
        *in_progress |= (1 << slot);
    }

    if (next_pixel(slot, iterations,&zx, &zy, &px, &py, baton))
    {
        cx->floats[slot] = px;
        cy->floats[slot] = py;
        zr->floats[slot] = zx;
        zi->floats[slot] = zy;
    }
    else
    {
        *in_progress &= ~(1 << slot);
    }

    if (*in_progress == 0)
    {
        return 0;
    }
    
    return 1;
}


void mfunc_simd_float(ALLOCATE_SLOTS allocate_slots, PIXEL_SOURCE next_pixel, PIXEL_OUTPUT output_pixel, BATON *baton)
{
    int i0 = 0;
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
    int in_progress = 0;

    __m128 cx;
    __m128 cy;
    __m128 zr;
    __m128 zi;
    __m128 zr2;
    __m128 zi2;
    __m128 t, t2;
    __m128 boundary;

    int_union test;
    
    int countdown_from;
    int countdown;

    allocate_slots(ENABLE_SLOT3 ? 4 : (ENABLE_SLOT2 ? 3 : (ENABLE_SLOT1 ? 2 : 1)), baton);
    
    boundary = _mm_set1_ps(2.0*2.0);
    cx = _mm_set1_ps(0.0);
    cy = _mm_set1_ps(0.0);
    zr = _mm_set1_ps(0.0);
    zi = _mm_set1_ps(0.0);

    while (1)
    {
        /* Check if it's time to output the first pixel and/or start a new one. */
        if (ENABLE_SLOT0 && !check_slot(0, &i0, &test, &in_progress, (float_union*) &cx, (float_union*) &cy, (float_union*) &zr, (float_union*) &zi, next_pixel, output_pixel, baton))
            break;

        /* Check if it's time to output the second pixel and/or start a new one. */
        if (ENABLE_SLOT1 && !check_slot(1, &i1, &test, &in_progress, (float_union*) &cx, (float_union*) &cy, (float_union*) &zr, (float_union*) &zi, next_pixel, output_pixel, baton))
            break;

        /* Check if it's time to output the third pixel and/or start a new one. */
        if (ENABLE_SLOT2 && !check_slot(2, &i2, &test, &in_progress, (float_union*) &cx, (float_union*) &cy, (float_union*) &zr, (float_union*) &zi, next_pixel, output_pixel, baton))
            break;

        /* Check if it's time to output the fourth pixel and/or start a new one. */
        if (ENABLE_SLOT3 && !check_slot(3, &i3, &test, &in_progress, (float_union*) &cx, (float_union*) &cy, (float_union*) &zr, (float_union*) &zi, next_pixel, output_pixel, baton))
            break;

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

        countdown_from = MIN(MIN(i0, i1), MIN(i2, i3));
        if (countdown_from <= 0)
            countdown_from = 1;
        countdown = countdown_from;

        while (1)
        {
            /* Do some work on the current pixel. */
            zr2 = _mm_mul_ps(zr, zr);
            zi2 = _mm_mul_ps(zi, zi);
            t = _mm_mul_ps(zr, zi);
            zr = _mm_sub_ps(zr2, zi2);
            zr = _mm_add_ps(zr, cx);
            zi = _mm_add_ps(t, t);
            zi = _mm_add_ps(zi, cy);

            countdown--;

            /* Check against the boundary. */
            t2 = _mm_add_ps(zr2, zi2);
            t2 = _mm_cmpgt_ps(t2, boundary);

            if (countdown == 0 || _mm_movemask_ps(t2))
                break;
        }

        test.m128 = t2;
        i0 -= (countdown_from - countdown);
        i1 -= (countdown_from - countdown);
        i2 -= (countdown_from - countdown);
        i3 -= (countdown_from - countdown);
    }
}
