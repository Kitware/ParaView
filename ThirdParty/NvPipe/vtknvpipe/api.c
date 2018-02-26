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
#include "config.nvp.h"
#include "debug.h"
#include "nvpipe.h"
#include "internal-api.h"

DECLARE_CHANNEL(api);

NVPIPE_VISIBLE nvpipe*
nvpipe_create_encoder(nvp_codec_t id, uint64_t bitrate) {
    switch(id) {
    case NVPIPE_H264_NV:
        return nvp_create_encoder(bitrate);
        break;
#if NVPIPE_FFMPEG == 1
    case NVPIPE_H264_NVFFMPEG:
        return nvp_create_ffmpeg(true, bitrate);
        break;
    case NVPIPE_H264_FFMPEG:
        return nvp_create_ffmpeg(false, bitrate);
        break;
#else
    case NVPIPE_H264_NVFFMPEG: /* fallthrough */
    case NVPIPE_H264_FFMPEG:
        ERR(api, "nvpipe: FFMpeg support not compiled in.");
        return NULL;
        break;
#endif
    }
    return NULL;
}

NVPIPE_VISIBLE nvpipe*
nvpipe_create_decoder(nvp_codec_t id) {
    switch(id) {
    case NVPIPE_H264_NV:
        return nvp_create_decoder();
        break;
#if NVPIPE_FFMPEG == 1
    case NVPIPE_H264_NVFFMPEG:
        return nvp_create_ffmpeg(true, 0);
        break;
    case NVPIPE_H264_FFMPEG:
        return nvp_create_ffmpeg(false, 0);
        break;
#else
    case NVPIPE_H264_NVFFMPEG: /* fallthrough */
    case NVPIPE_H264_FFMPEG:
        ERR(api, "nvpipe: FFMpeg support not compiled in.");
        return NULL;
#endif
    }
    return NULL;
}

nvp_err_t
nvpipe_encode(nvpipe* const __restrict cdc,
              const void* const __restrict ibuf,
              const size_t ibuf_sz,
              void* const __restrict obuf,
              size_t* const __restrict obuf_sz,
              const uint32_t width, const uint32_t height, nvp_fmt_t format) {
    assert(cdc);
    nvp_impl_t* enc = (nvp_impl_t*)cdc;
    return enc->encode(enc, ibuf,ibuf_sz, obuf,obuf_sz, width,height, format);
}

nvp_err_t
nvpipe_decode(nvpipe* const __restrict codec,
              const void* const __restrict ibuf,
              const size_t ibuf_sz,
              void* const __restrict obuf,
              uint32_t width,
              uint32_t height) {
    assert(codec);
    nvp_impl_t* dec = (nvp_impl_t*)codec;
    return dec->decode(dec, ibuf,ibuf_sz, obuf, width,height);
}

void
nvpipe_destroy(nvpipe* const __restrict codec) {
    if(codec == NULL) {
        return;
    }
    nvp_impl_t* nvp = (nvp_impl_t*)codec;
    nvp->destroy(nvp);
}

nvp_err_t
nvpipe_bitrate(nvpipe* const __restrict codec, uint64_t br) {
    assert(codec);
    if(codec == NULL) {
        return NVPIPE_EINVAL;
    }
    nvp_impl_t* nvp = (nvp_impl_t*)codec;
    return nvp->bitrate(codec, br);
}
