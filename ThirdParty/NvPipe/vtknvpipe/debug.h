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
#ifndef NVPIPE_DEBUG_H
#define NVPIPE_DEBUG_H
/** Simple debug channel implementation.  Example usage:
 *
 *   DECLARE_CHANNEL(stuff);
 *   TRACE(stuff, "is happening!");
 *   ERR(stuff, "something really bad happened.");
 *   WARN(stuff, "i think something's wrong?");
 *
 * The user can enable/disable the above channel by setting the NVPIPE_VERBOSE
 * environment variable:
 *
 *   export NVPIPE_VERBOSE="stuff=+err,-warn,+trace" */
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum _nvDbgChannelClass {
    Err=0,
    Warn,
    Trace,
    Fixme,
};

struct nvdbgchannel {
    unsigned flags;
    char name[32];
};

#define DEFAULT_CHFLAGS \
    (1U << Err) | (1U << Warn) | (1U << Fixme)
/* creates a new debug channel.  debug channels are private to implementation,
 * and must not be declared in header files. */
#ifdef _MSC_VER
#define DECLARE_CHANNEL(ch) \
    static struct nvdbgchannel nv_chn_##ch = { DEFAULT_CHFLAGS, #ch }; \
    static void \
    ch_init_##ch() { \
    const char* dbg_ = getenv("NVPIPE_VERBOSE"); \
    nv_parse_options(&nv_chn_##ch, dbg_); \
}
#else
#define DECLARE_CHANNEL(ch) \
    static struct nvdbgchannel nv_chn_##ch = { DEFAULT_CHFLAGS, #ch }; \
    __attribute__((constructor(200))) static void \
    ch_init_##ch() { \
    const char* dbg_ = getenv("NVPIPE_VERBOSE"); \
    nv_parse_options(&nv_chn_##ch, dbg_); \
}
#endif

#define TRACE(ch, ...) \
    nv_dbg(Trace, &nv_chn_##ch, __FUNCTION__, __VA_ARGS__)
#define ERR(ch, ...) \
    nv_dbg(Err, &nv_chn_##ch, __FUNCTION__, __VA_ARGS__)
#define WARN(ch, ...) \
    nv_dbg(Warn, &nv_chn_##ch, __FUNCTION__, __VA_ARGS__)
#define FIXME(ch, ...) \
    nv_dbg(Fixme, &nv_chn_##ch, __FUNCTION__, __VA_ARGS__)

/* for internal use only. */
void nv_dbg(enum _nvDbgChannelClass, const struct nvdbgchannel*,
            const char* func, const char* format, ...)
#ifdef __GNUC__
__attribute__((format(printf, 4, 5)));
#else
;
#endif
void nv_parse_options(struct nvdbgchannel*, const char* opt);

#ifdef __cplusplus
}
#endif

#endif
