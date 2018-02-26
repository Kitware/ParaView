/* Copyright (c) 2016-2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Performance note: Typically the color space conversions take up a
 * negligible amount of run time. The following kernels have therefore
 * not been optimized.
 */

#include <cassert>
#include <cstddef>
#include <cinttypes>
#include <device_functions.h>
#include <cuda.h>

static inline __device__ float
clamp(const float v, const float low, const float high) {
    return v < low ? low : v > high ? high : v;
}

static inline __device__ float
rgb2y(const uint8_t r, const uint8_t g, const uint8_t b) {
    return 0.299f*(float)r + 0.587f*(float)g + 0.114f*(float)b;
}
static inline __device__ float
rgb2u(const uint8_t r, const uint8_t g, const uint8_t b) {
    const float y = rgb2y(r,g,b);
    return clamp(-(((-(float)b + y) / 1.732446f) - 128.f), 0.f, 255.f);
}
static inline __device__ float
rgb2v(const uint8_t r, const uint8_t g, const uint8_t b) {
    const float y = rgb2y(r,g,b);
    const float u = rgb2u(r,g,b);
    return clamp((y - (0.337633f*(u-128.f)) - (float)g) / 0.698001f + 128.f,
                 0.f, 255.f);
}

/* Converts from RGB data to NV12.  NV12's "U" and "V" channels are interleaved
 * and subsampled 2x2.  Note the RGB data are not pitched. */
extern "C" __global__ void
rgb2yuv(const uint8_t* const __restrict rgb,
        const uint32_t width, const uint32_t height,
        const uint32_t widthUser, const uint32_t heightUser, const uint32_t c/*omponents*/,
        uint8_t* const __restrict yuv, unsigned pitch) {
    const uint32_t x = blockIdx.x*blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y*blockDim.y + threadIdx.y;
    const uint32_t i = y*pitch + x;

    if(x >= width || y >= height || i >= pitch*height)
        return;

    /* Repeat edge pixels for padded areas */
    const uint32_t _x = min(x, widthUser - 1);
    const uint32_t _y = min(y, heightUser - 1);
    const uint32_t j = _y * widthUser + _x;

    assert(pitch >= width);
    assert(i < pitch * height);
    assert(j < widthUser * heightUser);
    assert(width <= 4096);
    assert(height <= 4096);
    assert(c == 3 || c == 4);
    assert(pitch <= 4096);

    uint8_t* __restrict Y = yuv;
    Y[i] = (uint8_t)clamp(rgb2y(rgb[j*c+0], rgb[j*c+1], rgb[j*c+2]), 0, 255);

    /* U+V are downsampled 2x per dimension.  So kill off 3 of every 4 threads
     * that reach here; only one will do the writes into U and V. */
    /* thought: use x0 to write into U and x1 to write into V, to spread load? */
    if(x&1 == 1 || y&1 == 1) {
        return;
    }
    uint8_t* __restrict uv = yuv + pitch*height;
    const uint32_t uvidx = y/2*(pitch/2) + x/2;

    const uint32_t idx[4] = {
        min((_y+0)*widthUser + _x+0, widthUser*heightUser - 1),
        min((_y+0)*widthUser + _x+1, widthUser*heightUser - 1),
        min((_y+1)*widthUser + _x+0, widthUser*heightUser - 1),
        min((_y+1)*widthUser + _x+1, widthUser*heightUser - 1),
    };
    const float u[4] = {
        rgb2u(rgb[idx[0]*c+0], rgb[idx[0]*c+1], rgb[idx[0]*c+2]),
        rgb2u(rgb[idx[1]*c+0], rgb[idx[1]*c+1], rgb[idx[1]*c+2]),
        rgb2u(rgb[idx[2]*c+0], rgb[idx[2]*c+1], rgb[idx[2]*c+2]),
        rgb2u(rgb[idx[3]*c+0], rgb[idx[3]*c+1], rgb[idx[3]*c+2])
    };
    const float v[4] = {
        rgb2v(rgb[idx[0]*c+0], rgb[idx[0]*c+1], rgb[idx[0]*c+2]),
        rgb2v(rgb[idx[1]*c+0], rgb[idx[1]*c+1], rgb[idx[1]*c+2]),
        rgb2v(rgb[idx[2]*c+0], rgb[idx[2]*c+1], rgb[idx[2]*c+2]),
        rgb2v(rgb[idx[3]*c+0], rgb[idx[3]*c+1], rgb[idx[3]*c+2])
    };
    uv[uvidx*2+0] = (uint8_t)clamp((u[0] + u[1] + u[2] + u[3]) / 4.0, 0, 255);
    uv[uvidx*2+1] = (uint8_t)clamp((v[0] + v[1] + v[2] + v[3]) / 4.0, 0, 255);
}

static inline __device__ float
yuv2r(const uint8_t y, const uint8_t u, const uint8_t v) {
    (void)u;
    return (y-16)*1.164f + (1.596f * (v-128));
}
static inline __device__ float
yuv2g(const uint8_t y, const uint8_t u, const uint8_t v) {
    return (y-16)*1.164f + (u-128)*-0.392f + (v-128)*-0.813f;
}
static inline __device__ float
yuv2b(const uint8_t y, const uint8_t u, const uint8_t v) {
    (void)v;
    return (y-16)*1.164f + (u-128)*2.017f;
}

/* Convert back from NV12 to RGB.   Note the RGB buffer is not pitched. */
extern "C" __global__ void
yuv2rgb(const uint8_t* const __restrict yuv,
        const uint32_t width, const uint32_t height,
        uint32_t widthUser, uint32_t heightUser, unsigned pitch,
        uint8_t* const __restrict rgb) {
    const uint32_t x = blockIdx.x*blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y*blockDim.y + threadIdx.y;
    const uint32_t i = y*pitch + x;
    const uint32_t j = y*widthUser + x;
    if(x >= widthUser || y >= heightUser || x >= width || y >= height || i >= pitch*height || j >= widthUser * heightUser) {
        return;
    }
    assert(i < pitch*height);
    assert(j < widthUser * heightUser);
    assert(width <= 4096);
    assert(height <= 4096);
    assert(pitch <= 4096);
    const uint8_t* __restrict Y = yuv;
    const uint8_t* __restrict uv = yuv + pitch*height;
    const uint32_t xx = min(x+1, width-1);
    const uint32_t yy = min(y+1, height-1);
    const uint32_t idx[4] = {
        y/2*pitch/2 + x/2,
        y/2*pitch/2 + xx/2,
        yy/2*pitch/2 + x/2,
        yy/2*pitch/2 + xx/2,
    };
    const uint8_t u[4] = {
        uv[idx[0]*2+0], uv[idx[1]*2+0], uv[idx[2]*2+0], uv[idx[3]*2+0]
    };
    const uint8_t v[4] = {
        uv[idx[0]*2+1], uv[idx[1]*2+1], uv[idx[2]*2+1], uv[idx[3]*2+1]
    };
    rgb[j*3+0] = clamp(
                (yuv2r(Y[i], u[0], v[0]) + yuv2r(Y[i], u[1], v[1]) +
            yuv2r(Y[i], u[2], v[2]) + yuv2r(Y[i], u[3], v[3])) / 4.0, 0, 255
            );
    rgb[j*3+1] = clamp(
                (yuv2g(Y[i], u[0], v[0]) + yuv2g(Y[i], u[1], v[1]) +
            yuv2g(Y[i], u[2], v[2]) + yuv2g(Y[i], u[3], v[3])) / 4.0, 0, 255
            );
    rgb[j*3+2] = clamp(
                (yuv2b(Y[i], u[0], v[0]) + yuv2b(Y[i], u[1], v[1]) +
            yuv2b(Y[i], u[2], v[2]) + yuv2b(Y[i], u[3], v[3])) / 4.0, 0, 255
            );
}

extern "C" cudaError_t
launch_yuv2rgb(CUdeviceptr nv12, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser, unsigned pitch,
               CUdeviceptr rgb, cudaStream_t strm) {
    /* NvCodec maxes out at 8k anyway. */
    assert(width <= 8192);
    assert(height <= 8192);
    /* NvCodec can't give us a height that isn't evenly divisible. */
    assert(height%2 == 0);
    const void* args[] = {
        (void*)&nv12, &width, &height, &widthUser, &heightUser, &pitch, (void*)&rgb, 0
    };
    const dim3 gdim = {(unsigned)(width/16)+1, (unsigned)(height/2), 1};
    const dim3 bdim = {16, 2, 1};
    const size_t shmem = 0;
    return cudaLaunchKernel((const void**)yuv2rgb, gdim, bdim, (void**)args,
                            shmem, strm);
}

extern "C" cudaError_t
launch_rgb2yuv(CUdeviceptr rgb, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser, uint32_t ncomp,
               CUdeviceptr nv12, unsigned pitch, cudaStream_t strm) {
    /* NvCodec maxes out at 8k anyway. */
    assert(width <= 8192);
    assert(height <= 8192);
    /* We only support RGB and RGBA data. */
    assert(ncomp == 3 || ncomp == 4);
    /* NvCodec can't give us a height that isn't evenly divisible. */
    assert(height%2 == 0);

    const void* args[] = {
        (void*)&rgb, &width, &height, &widthUser, &heightUser, &ncomp, (void*)&nv12, &pitch,
    };
    dim3 gdim = {(unsigned)(width/16)+1, (unsigned)(height/2), 1};
    dim3 bdim = {16, 2, 1};
    const size_t shmem = 0;
    return cudaLaunchKernel((const void*)rgb2yuv, gdim, bdim, (void**)args,
                            shmem, strm);
}
