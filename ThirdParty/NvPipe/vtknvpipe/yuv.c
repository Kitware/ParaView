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
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <cuda_runtime_api.h>
#include <nvToolsExt.h>
#include <nvToolsExtCuda.h>
#include "config.nvp.h"
#include "debug.h"
#include "yuv.h"

DECLARE_CHANNEL(yuv);

static cudaError_t
strm_sync(void* f) {
    nv_fut_t* fut = (nv_fut_t*)f;
    const cudaError_t sy = cudaStreamSynchronize(fut->strm);
    if(cudaSuccess != sy) {
        ERR(yuv, "Error %d synchronizing stream", sy);
    }
    return sy;
}

static void
strm_destroy(void* f) {
    if(f == NULL) {
        return;
    }
    nv_fut_t* fut = (nv_fut_t*)f;
    const cudaError_t del = cudaStreamDestroy(fut->strm);
    if(cudaSuccess != del) {
        WARN(yuv, "Error %d destroying stream.", del);
    }
    fut->strm = 0;
}

static nv_fut_t
strm_create() {
    nv_fut_t rv = {0};
    rv.sync = strm_sync;
    rv.destroy = strm_destroy;

    const cudaError_t cuerr = cudaStreamCreateWithFlags(&rv.strm,
                                                        cudaStreamNonBlocking);
    if(cudaSuccess != cuerr) {
        ERR(yuv, "Error %d creating stream.", cuerr);
        return rv;
    }
    return rv;
}

typedef struct rgb_convert {
    nv_fut_t fut;
    uint32_t components;
} rgb2yuv_t;

extern cudaError_t
launch_rgb2yuv(CUdeviceptr rgb, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser, uint32_t ncomp,
               CUdeviceptr nv12, unsigned pitch, cudaStream_t strm);

static cudaError_t
rgb2yuv_submit(void* conv, const CUdeviceptr rgb, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser,
               CUdeviceptr nv12, unsigned pitch) {
    rgb2yuv_t* cnv = (rgb2yuv_t*)conv;
    return launch_rgb2yuv(rgb, width, height, widthUser, heightUser, cnv->components, nv12, pitch,
                          cnv->fut.strm);
}

static void
rgb2yuv_destroy(void* r) {
    if(r == NULL) {
        return;
    }
    rgb2yuv_t* conv = (rgb2yuv_t*)r;
    strm_destroy(&conv->fut);
    memset(conv, 0, sizeof(rgb2yuv_t));
    free(conv);
}

static nv_fut_t*
rgb2yuv_create(uint32_t components) {
    rgb2yuv_t* rv = calloc(1, sizeof(rgb2yuv_t));
    rv->fut = strm_create();
    nvtxNameCuStream(rv->fut.strm, "encode");
    rv->fut.submit = rgb2yuv_submit;
    /* Overwrite destructor with ours. */
    rv->fut.destroy = rgb2yuv_destroy;
    rv->components = components;

    return (nv_fut_t*)rv;
}

typedef struct yuv_convert {
    nv_fut_t fut;
} yuv2rgb_t;

extern cudaError_t
launch_yuv2rgb(CUdeviceptr nv12, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser, unsigned pitch,
               CUdeviceptr rgb, cudaStream_t strm);

static cudaError_t
yuv2rgb_submit(void* y, const CUdeviceptr nv12, uint32_t width, uint32_t height,
               uint32_t widthUser, uint32_t heightUser, CUdeviceptr rgb, unsigned pitch) {
    yuv2rgb_t* conv = (yuv2rgb_t*)y;
    return launch_yuv2rgb(nv12, width, height, widthUser, heightUser, pitch, rgb, conv->fut.strm);
}

static void
yuv2rgb_destroy(void* y) {
    if(y == NULL) {
        return;
    }
    yuv2rgb_t* conv = (yuv2rgb_t*)y;
    strm_destroy(&conv->fut);
    memset(conv, 0, sizeof(yuv2rgb_t));
    free(conv);
}

static nv_fut_t*
yuv2rgb_create() {
    yuv2rgb_t* rv = calloc(1, sizeof(yuv2rgb_t));
    rv->fut = strm_create();
    nvtxNameCuStream(rv->fut.strm, "decode");
    rv->fut.submit = yuv2rgb_submit;
    /* Overwrite destructor with ours. */
    rv->fut.destroy = yuv2rgb_destroy;
    return (nv_fut_t*)rv;
}

nv_fut_t*
rgb2nv12(uint32_t components) {
    return rgb2yuv_create(components);
}
nv_fut_t* nv122rgb() {
    return yuv2rgb_create();
}
