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
#ifndef NVPIPE_H_
#define NVPIPE_H_

#include <stdlib.h>
#include <stdint.h>
#include "mangle_nvpipe.h"
#ifdef __cplusplus
extern "C" {
#endif

/** Codecs usable for the encoding/decoding session */
typedef enum nvpipe_codec {
    NVPIPE_H264_NV, /**< NVIDIA video codec SDK backend */
    NVPIPE_H264_NVFFMPEG, /**< NVIDIA-based ffmpeg backend */
    NVPIPE_H264_FFMPEG, /**< CPU-based ffmpeg backend */
} nvp_codec_t;

/** Supported NvPipe image formats. */
typedef enum nvpipe_format {
    NVPIPE_RGB,
    NVPIPE_RGBA,
} nvp_fmt_t;

/* Avoid a dependency on cuda.h by copying these definitions here. */
#define cuda_SUCCESS 0
#define cuda_ERROR_INVALID_VALUE 11
#define cuda_ERROR_OUT_OF_MEMORY 2
#define cuda_ERROR_MAP_FAILED 14
#define cuda_ERROR_UNMAP_FAILED 15
#define cuda_ERROR_FILE_NOT_FOUND 33
#define cuda_ERROR_UNKNOWN 10000

/** NvPipe error codes are a superset of the CUDA error codes.  See
 * nvpipe_strerror. */
typedef enum nvpipe_error_code {
    NVPIPE_SUCCESS = cuda_SUCCESS,
    NVPIPE_EINVAL = cuda_ERROR_INVALID_VALUE,
    NVPIPE_ENOMEM = cuda_ERROR_OUT_OF_MEMORY,
    NVPIPE_EMAP = cuda_ERROR_MAP_FAILED,
    NVPIPE_EUNMAP = cuda_ERROR_UNMAP_FAILED,
    NVPIPE_ENOENT = cuda_ERROR_FILE_NOT_FOUND,
    NVPIPE_EENCODE = cuda_ERROR_UNKNOWN+1,
    NVPIPE_EDECODE = cuda_ERROR_UNKNOWN+2,
    NVPIPE_EOVERFLOW = cuda_ERROR_UNKNOWN+3,
    NVPIPE_EAGAIN = cuda_ERROR_UNKNOWN+4,
} nvp_err_t;

typedef void nvpipe;

#ifdef __GNUC__
#	define NVPIPE_VISIBLE __attribute__((visibility("default")))
#else
#	define NVPIPE_VISIBLE __declspec(dllexport)
#endif

/** @fn create nvpipe instance
 *
 * return a valid instance on success, NULL otherwise.
 * @param[in] backend implementation to use.
 * @param[in] bitrate rate to use
 *
 * If you're unsure what to use for a bitrate, we suggest the Kush gauge:
 *  [image width] x [image height] x [framerate] x [motion rank] x 0.07
 *      [motion rank]:  1 being low motion;
 *                      2 being medium motion;
 *                      4 being high motion;
 * See also:
 *      http://www.adobe.com/content/dam/Adobe/en/devnet/
 */
NVPIPE_VISIBLE nvpipe*
nvpipe_create_encoder(nvp_codec_t id, uint64_t bitrate);

/** @fn create nvpipe decoding instance
 *
 * return a valid instance on success, NULL otherwise.
 * @param[in] backend implementation to use.
 */
NVPIPE_VISIBLE nvpipe*
nvpipe_create_decoder(nvp_codec_t id);

/** \brief free nvpipe instance
 *
 * clean up each instance created by 'nvpipe_create_*()'.
 */
NVPIPE_VISIBLE void
nvpipe_destroy(nvpipe* const __restrict codec);

/** encode/compress images
 *
 * User provides pointers for both input and output buffers.  The output buffer
 * must be large enough to accommodate the compressed data.  Sizing the output
 * buffer may be difficult; the call will return OUTPUT_BUFFER_OVERFLOW to
 * indicate that the user must increase the size of the buffer.
 * On successful execution, the output buffer size will be modified to indicate
 * how many bytes were actually filled.
 *
 * @param[in] codec library instance to use
 * @param[in] ibuf input buffer to compress. host or device pointer.
 * @param[in] ibuf_sz number of bytes in the input buffer
 * @param[out] obuf (host) buffer to place compressed data into.
 * @param[in,out] obuf_sz number of bytes available in 'obuf', output is number
 *                        of bytes that were actually filled.
 * @param[in] width number of pixels in X of the input buffer
 * @param[in] height number of pixels in Y of the input buffer
 * @param[in] format the format of ibuf.
 *
 * @return NVPIPE_SUCCESS on success, nonzero on error.
 */
NVPIPE_VISIBLE nvp_err_t
nvpipe_encode(nvpipe * const __restrict codec,
              const void *const __restrict ibuf,
              const size_t ibuf_sz,
              void *const __restrict obuf,
              size_t* const __restrict obuf_sz,
              const uint32_t width, const uint32_t height,
              nvp_fmt_t format);

/** Adjust the bitrate used for an encoder.  The setting takes effect for
 * subsequent frames.
 *
 * The bitrate determines the quality of the encoded image.  NvPipe will
 * automatically adjust to new image sizes, but it will not update the bitrate
 * it utilizes without (this) explicit request.
 *
 * Note this only makes sense for encoders; decoders accept streams encoded at
 * any bitrate. */
NVPIPE_VISIBLE
nvp_err_t nvpipe_bitrate(nvpipe* const enc, uint64_t bitrate);

/** decode/decompress a frame
 *
 * @param[in] codec instance variable
 * @param[in] ibuf the compressed frame, on the host.
 * @param[in] ibuf_sz  the size in bytes of the compressed data
 * @param[out] obuf where the output RGB frame will be written.
 *             must be at least width*height*3 bytes. host or device pointer.
 * @param[in] width  width of output image
 * @param[in] height height of output image
 *
 * @return NVPIPE_SUCCESS on success, nonzero on error.
 */
NVPIPE_VISIBLE nvp_err_t
nvpipe_decode(nvpipe* const __restrict codec,
              const void* const __restrict ibuf,
              const size_t ibuf_sz,
              void* const __restrict obuf,
              uint32_t width,
              uint32_t height);

/** Retrieve human-readable error message for the given error code.  Note that
 * this is a pointer to constant memory that must NOT be freed or manipulated
 * by the user. */
NVPIPE_VISIBLE const char*
nvpipe_strerror(nvp_err_t error_code);

#  ifdef __cplusplus
}
#  endif
#endif
