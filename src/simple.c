#include "fractal.h"

#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>


#define PIXEL_COST 50
#define QUOTA_SIZE 500000


typedef struct DRAWING
{
    WINDOW *window;
    FRACTAL *fractal;
    MFUNC *mfunc;
    GET_POINT *get_point;
    int *x_slots;
    int *y_slots;
    int i, j;
    int quota;
    int width, height;
} DRAWING;


DRAWING *simple_create(WINDOW *window, FRACTAL *fractal, GET_POINT get_point, MFUNC *mfunc)
{
    DRAWING *drawing = malloc(sizeof(DRAWING));
    if (!drawing)
    {
        fprintf(stderr, "%s:%d: Can't create drawing!", __FILE__, __LINE__);
        return NULL;
    }
    drawing->window = window;
    drawing->fractal = fractal;
    drawing->mfunc = mfunc;
    drawing->get_point = get_point;
    drawing->width = window->width;
    drawing->height = window->height;
    drawing->i = 0;
    drawing->j = 0;
    drawing->x_slots = NULL;
    drawing->y_slots = NULL;
    return drawing;
}


static void simple_allocate_slots(int num_slots, BATON *baton)
{
    DRAWING *drawing = (DRAWING *) baton;
    
    drawing->x_slots = malloc(sizeof(int) * num_slots);
    drawing->y_slots = malloc(sizeof(int) * num_slots);
    if (!drawing->x_slots || !drawing->y_slots)
    {
        fprintf(stderr, "Can't create slots!");
        exit(1);
    }
}


static int simple_next_pixel(int slot, int *max_iterations, double *zx, double *zy, double *cx, double *cy, BATON *baton)
{
    DRAWING *drawing = (DRAWING *) baton;
    
    if (drawing->quota <= 0 || drawing->i >= drawing->height)
        return 0;
    
    drawing->get_point(drawing->fractal, drawing->j, drawing->i, zx, zy, cx, cy);

    drawing->x_slots[slot] = drawing->j;
    drawing->y_slots[slot] = drawing->i;
    
    drawing->j++;

    if (drawing->j >= drawing->width)
    {
        drawing->j = 0;
        drawing->i++;
    }
    
    *max_iterations = drawing->window->depth;

    return 1;
}


static void simple_output_pixel(int slot, int remaining, double fx, double fy, BATON *baton)
{
    DRAWING *drawing = (DRAWING *) baton;
    int k;
    
    if (remaining == 0)
    {
        k = 0;
    }
    else
    {
        k = drawing->window->depth - remaining;
    }
    
    set_pixel(drawing->window, drawing->x_slots[slot], drawing->y_slots[slot], k, fx, fy);
    drawing->quota -= ((k == 0) ? drawing->window->depth : k) + PIXEL_COST;
}


void simple_update(DRAWING *drawing)
{
    drawing->quota = QUOTA_SIZE;

    drawing->mfunc(simple_allocate_slots, simple_next_pixel, simple_output_pixel, (BATON *) drawing);
    
    status = "RENDERING";
}


void simple_destroy(DRAWING *drawing)
{
    free(drawing->x_slots);
    free(drawing->y_slots);
    free(drawing);
}
