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
#ifndef NVPIPE_YUV_H
#define NVPIPE_YUV_H

#include <stdbool.h>
#include <cuda_runtime_api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef cudaError_t (fut_fqn_submit)(void* obj, const CUdeviceptr src,
                                     uint32_t width, uint32_t height,
                                     uint32_t widthUser, uint32_t heightUser,
                                     CUdeviceptr dst, unsigned pitch);
typedef cudaError_t (fut_fqn_sync)(void*);
typedef cudaStream_t (fut_fqn_stream)(const void*);
typedef void (fut_fqn_destroy)(void*);

/** Future abstraction for data reorganization/conversion.
 * Usage:
 *   1. create the future via some creation function
 *   2. submit() its workload.  similar to 'execute' in a vis pipeline.
 *   3. use 'strm' to enqueue your own post-workload GPU work
 *   4. sync() when you need the result.
 *   5. goto 2 to have it reorganize another frame.
 *   6. destroy(self) when you are done to clean everything up. */
typedef struct cu_convert_future {
    /** Converts a WIDTHxHEIGHT image between nv12 and RGB[A] formats.
     * NV12 organization is WxH bytes of the Y channel, followed by 2*(W/2xH/2)
     * bytes of the interleaved U and V channels.
     * @param obj future you are submitting this into
     * @param src the nv12 or RGB[A] data you wish to convert
     * @param width the width of the input and output image
     * @param height the height of the input and output image
     * @param pitch the pitch for nv12 memory. RGB[A] memory is unpitched!
     * @param dst the output buffer. */
    fut_fqn_submit* submit;
    /** Synchronize the stream used to submit work.  You must call this before
     * using the 'dst' output of submit().
     * This is just cuStreamSynchronize.  You could submit() multiple frames
     * before sync()ing, if desired. */
    fut_fqn_sync* sync;
    /** Clean up internal resources. */
    fut_fqn_destroy* destroy;
    /** The stream work will be submitted under. */
    cudaStream_t strm;
} nv_fut_t;

/** a future that reorganizes RGB[A] data into nv12 data.
 * @param components the number of components: 3 or 4. */
nv_fut_t* rgb2nv12(uint32_t components);
/** a future that reorganizes nv12 data into RGB data. */
nv_fut_t* nv122rgb();

#ifdef __cplusplus
}
#endif

#endif
