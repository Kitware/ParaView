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
#include "nvpipe.h"

struct errstr {
    nvp_err_t code;
    const char* msg;
};
static const struct errstr nvp_errors[] = {
{ NVPIPE_SUCCESS, "success" },
{ NVPIPE_EINVAL, "invalid value"},
{ NVPIPE_ENOMEM, "out of memory"},
{ NVPIPE_EMAP, "map resource"},
{ NVPIPE_EUNMAP, "unmap resource"},
{ NVPIPE_ENOENT, "file or resource not found"},
{ NVPIPE_EENCODE, "encode error from NvCodec"},
{ NVPIPE_EDECODE, "decode error from NvCodec"},
{ NVPIPE_EOVERFLOW, "buffer would overflow"},
{ NVPIPE_EAGAIN, "not ready yet"},
};

const char*
nvpipe_strerror(nvp_err_t ecode) {
    const size_t n = sizeof(nvp_errors) / sizeof(nvp_errors[0]);
    for(size_t i=0; i < n; ++i) {
        if(ecode == nvp_errors[i].code) {
            return nvp_errors[i].msg;
        }
    }
    return "unknown";
}
