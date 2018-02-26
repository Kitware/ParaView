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
#ifndef NVPIPE_INTERNAL_API_H
#define NVPIPE_INTERNAL_API_H

#include <stdbool.h>
#include "config.nvp.h"
#include "nvpipe.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef nvp_err_t (fqn_encode)(
        nvpipe * const __restrict codec,
        const void *const __restrict ibuf,
        const size_t ibuf_sz,
        void *const __restrict obuf,
        size_t* const __restrict obuf_sz,
        const uint32_t width, const uint32_t height,
        nvp_fmt_t format
        );
typedef nvp_err_t (fqn_bitrate)(nvpipe* codec, uint64_t);
typedef nvp_err_t (fqn_decode)(
        nvpipe* const __restrict codec,
        const void* const __restrict ibuf, const size_t ibuf_sz,
        void* const __restrict obuf,
        uint32_t width, uint32_t height
        );
typedef void (fqn_destroy)(nvpipe* const __restrict);

enum objtype {
    ENCODER=0,
    DECODER,
#if NVPIPE_FFMPEG == 1
    FFMPEG
#endif
};

typedef struct nvp_impl_ {
    enum objtype type;
    fqn_encode* encode;
    fqn_bitrate* bitrate;
    fqn_decode* decode;
    fqn_destroy* destroy;
} nvp_impl_t;

nvp_impl_t* nvp_create_encoder(uint64_t bitrate);
nvp_impl_t* nvp_create_decoder();
nvp_impl_t* nvp_create_ffmpeg(bool nvidia, uint64_t bitrate);

#ifdef __cplusplus
}
#endif

#endif
