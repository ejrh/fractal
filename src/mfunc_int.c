#include "fractal.h"

/** Type of fixed-point value. */
typedef long int FIXED;

/** Semi-scaling factor, which is applied to each multiplicand when
 * multiplying, so that the same precision is discarded on each, and the
 * product fits within range.
 *
 * 7723 was chosen to minimise artefacts around the boundary; otherwise one
 * occurs at position 2+0i.  A bit of calculation shows the sequence of
 * values:
 *   - b = 4; z = 0; c = 2
 *   - z = z^2 + c = 2
 *   - z = z^2 + c = 6 
 *   - z = z^2 + c = 38
 * where 6 > 4 of course, however the next z is already calculated at
 * that point.
 *
 * But z*FIXED_SCALE = 36*7724*7724 = 2147210244, which comes close to
 * overflowing the range of FIXED.  This value is used in the colour
 * interpolations.  If the value overflows, strange colours are used for that
 * pixel's calculation.  Depending on the excessiveness of FIXED_SEMI_SCALE,
 * the artefacts will be confined to small areas around the boundary or may
 * effectively overwhelm the whole fractal, giving an interesting "crop
 * circle" pattern.
 *
 * Part of the problem is the phase mismatch between z and z^2 in the cycle.
 * However this is a characteristic of all mfuncs and helps with smoothing
 * the interpolation.
 */
#define FIXED_SEMI_SCALE 7723

/** Scaling factor.  A fixed-point value X is represented by the integer X*FIXED_SCALE. */
#define FIXED_SCALE (FIXED_SEMI_SCALE*FIXED_SEMI_SCALE)

/** Convert a native value (e.g. a double) to a fixed-point one. */
#define TO_FIXED(x) ((long int) ((x) * FIXED_SCALE))

/** Multiply two fixed-point values.  Note that the same precision is discarded on each when multiplying. */
#define FIXED_TIMES(x, y) ((x) / FIXED_SEMI_SCALE) * ((y) / FIXED_SEMI_SCALE)

/** Convert a fixed-point value to a native one (e.g. a double). */
#define FROM_FIXED(x) ((x) / (double) FIXED_SCALE)


void mfunc_loop_int(ALLOCATE_SLOTS allocate_slots, PIXEL_SOURCE next_pixel, PIXEL_OUTPUT output_pixel, BATON *baton)
{
    int i = 0;
    FIXED boundary = TO_FIXED(2.0*2.0);
    FIXED zr, zi;
    FIXED zr2 = boundary, zi2 = boundary;
    FIXED cr, ci;
    int done = 0;
    
    allocate_slots(1, baton);
    
    while (1)
    {
        FIXED t;
        
        /* Check if it's time to output a pixel and/or start a new one. */
        if (i <= 0 || zr2 + zi2 > boundary)
        {
            double zx, zy, cx, cy;
            
            if (done != 0)
            {
                if (zr2 + zi2 <= boundary)
                    output_pixel(0, 0, FROM_FIXED(zr), FROM_FIXED(zi), baton);
                else
                    output_pixel(0, i, FROM_FIXED(zr), FROM_FIXED(zi), baton);
            }
            
            if (!next_pixel(0, &i, &zx, &zy, &cx, &cy, baton))
                break;
            
            zr = TO_FIXED(zx);
            zi = TO_FIXED(zy);
            cr = TO_FIXED(cx);
            ci = TO_FIXED(cy);
            
            done += 1;
        }
    
        /* Do some work on the current pixel. */
        zr2 = FIXED_TIMES(zr, zr);
        zi2 = FIXED_TIMES(zi, zi);
        t = FIXED_TIMES(zr, zi);
        zr = zr2 - zi2 + cr;
        zi = t + t + ci;

        i--;
    }
}
