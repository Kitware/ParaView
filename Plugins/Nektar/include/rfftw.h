/*
 * Copyright (c) 1997 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use, copy, modify, and distribute the Software without
 * restriction, provided the Software, including any modified copies made
 * under this license, is not distributed for a fee, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE MASSACHUSETTS INSTITUTE OF TECHNOLOGY BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Massachusetts
 * Institute of Technology shall not be used in advertising or otherwise
 * to promote the sale, use or other dealings in this Software without
 * prior written authorization from the Massachusetts Institute of
 * Technology.
 *
 */

#ifndef RFFTW_H
#define RFFTW_H

#include <fftw.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* RFFTW: real<->complex transform in one dimension */

typedef struct {
    fftw_plan plan;
    fftw_twiddle *tw;
    int which_case;
} rfftw_plan_struct;

typedef rfftw_plan_struct *rfftw_plan;

typedef enum {
    REAL_TO_COMPLEX,
    COMPLEX_TO_REAL
} rfftw_type;

rfftw_plan rfftw_create_plan(int n, fftw_direction dir, int flags, rfftw_type type);

void rfftw_destroy_plan(rfftw_plan plan);

void rfftw(rfftw_plan plan, int howmany,
     FFTW_COMPLEX * in, int istride, int idist,
     FFTW_COMPLEX * out, int ostride, int odist);

/* RFFTWND: real<->complex transform in multiple dimensions */

typedef struct {
    rfftw_type type;
    rfftw_plan rplan;
    fftwnd_plan plan_nd;
    int real_n;
} rfftwnd_plan_struct;

typedef rfftwnd_plan_struct *rfftwnd_plan;

/* Initializing the RFFTWND Auxiliary Data */
rfftwnd_plan rfftw2d_create_plan(int nx, int ny, fftw_direction dir, int flags, rfftw_type type);
rfftwnd_plan rfftw3d_create_plan(int nx, int ny, int nz,
       fftw_direction dir, int flags, rfftw_type type);
rfftwnd_plan rfftwnd_create_plan(int rank, int *n, fftw_direction dir,
         int flags, rfftw_type type);

/* Freeing the RFFTWND Auxiliary Data */
void rfftwnd_destroy_plan(rfftwnd_plan plan);

/* Computing the real-complex N-Dimensional FFT */
void rfftwnd(rfftwnd_plan plan, int howmany,
       FFTW_COMPLEX * in, int istride, int idist,
       FFTW_COMPLEX * out, int ostride, int odist);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RFFTW_H */
