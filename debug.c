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
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#ifdef _MSC_VER
#	include "winposix.h"
#else
#	include <strings.h>
#	include <unistd.h>
#endif
#include "debug.h"

static long pid = -1;
static bool color_enabled = false;

#ifdef __GNUC__
__attribute__((constructor(101)))
#endif
static void
fp_dbg_init() {
    pid = (long)getpid();
#ifdef _WIN32
    color_enabled = isatty(_fileno(stdout)) == 1;
#else
    color_enabled = isatty(STDOUT_FILENO) == 1;
#endif
}

static bool
dbgchannel_enabled(const struct nvdbgchannel *chn,
                   enum _nvDbgChannelClass c) {
    return (chn->flags & (1U << c)) > 0;
}

/* ANSI escape codes for colors. */
static const char* C_NORM   = "\033[00m";
static const char* C_RED    = "\033[01;31m";
static const char* C_YELLOW = "\033[01;33m";
static const char* C_LBLUE  = "\033[01;36m";
static const char* C_WHITE = "\033[01;27m";
/* Might want these in the future ... */
#if 0
static const char* C_DGRAY = "\033[01;30m";
static const char* C_GREEN = "\033[01;32m";
static const char* C_MAG   = "\033[01;35m";
#endif

static const char*
color(const enum _nvDbgChannelClass cls) {
    if(!color_enabled) {
        return "";
    }
    switch (cls) {
    case Trace:
        return C_WHITE;
    case Warn:
        return C_YELLOW;
    case Err:
        return C_RED;
    case Fixme:
        return C_LBLUE;
    }
    assert(false);
    return C_NORM;
}

void
nv_dbg(enum _nvDbgChannelClass type, const struct nvdbgchannel *channel,
       const char* func, const char* format, ...) {
    va_list args;
    va_start(args, format);
    if(dbgchannel_enabled(channel, type)) {
        const char* fixit = type == Fixme ? "-FIXME" : "";
        printf("%s[%ld](%s%s) ", color(type), pid, func, fixit);
        (void)vprintf(format, args);
        printf("%s\n", color_enabled ? C_NORM : "");
    }
    va_end(args);
}

/* maps a string name to a class.  there should be a one-to-one mapping from
 * every entry in 'enum _nvDbgChannelClass' to this. */
static enum _nvDbgChannelClass
        name_class(const char* name) {
    if(strncasecmp(name, "err", 3) == 0) {
        return Err;
    }
    if(strncasecmp(name, "warn", 4) == 0) {
        return Warn;
    }
    if(strncasecmp(name, "trace", 5) == 0) {
        return Trace;
    }
    if(strncasecmp(name, "fixme", 5) == 0) {
        return Fixme;
    }
    assert(false);
    /* hack.  what do we do if they give us a class that isn't defined?  well,
     * since we use this to find the flag's position by bit-shifting, let's just
     * do something we know will shift off the end of our flag sizes.  that way,
     * undefined classes are just silently ignored. */
    return 64;
}

/* parses options of the form "chname=+a,-b,+c;chname2=+d,-c". */
void
nv_parse_options(struct nvdbgchannel *ch, const char* opt) {
#ifdef _MSC_VER
    static_assert(sizeof(enum _nvDbgChannelClass) <= sizeof(unsigned),
              #else
    _Static_assert(sizeof(enum _nvDbgChannelClass) <= sizeof(unsigned),
               #endif
                   "to make sure we can't shift beyond flags");
    /* special case: if the environment variable is simply "1", then turn
     * everything on. */
    if(opt && strcmp(opt, "1") == 0) {
        ch->flags = (1U << Err) | (1U << Warn) | (1U << Fixme) | (1U << Trace);
        return;
    }
    /* outer loop iterates over channels.  channel names are separated by ';' */
    for(const char* chan = opt; chan && chan != (const char*)0x1;
        chan = strchr(chan, ';') + 1) {
        /* extract a substring to make parsing easier. */
        char* chopts = strdup(chan);
        { /* if there's another channel after, cut the string there. */
            char* nextopt = strchr(chopts, ';');
            if(nextopt) {
                *nextopt = '\0';
            }
        }
        if(strncmp(chopts, ch->name, strlen(ch->name)) == 0) {
            /* matched our channel name.  now we want to parse the list of options,
             * separated by commas, e.g.: "+x,-y,+blah,+abc" */
            for(char* olist = strchr(chopts, '=') + 1;
                olist && olist != (const char*)0x1; olist = strchr(olist, ',') + 1) {
                /* the "+1" gets rid of the minus or plus */
                enum _nvDbgChannelClass cls = name_class(olist + 1);
                /* temporarily null out the subsequent options, for printing. */
                char* optend = strchr(olist, ',');
                if(optend) {
                    *optend = '\0';
                }
                if(*olist == '+') {
                    fprintf(stderr, "[%ld] %s: enabling %s\n", pid, ch->name, olist + 1);
                    ch->flags |= (1U << (uint16_t) cls);
                } else if(*olist == '-') {
                    fprintf(stderr, "[%ld] %s: disabling %s\n", pid, ch->name, olist + 1);
                    ch->flags &= ~(1U << (uint16_t) cls);
                }
                /* 'de-null' it. */
                if(optend) {
                    *optend = ',';
                }
            }
        }
        free(chopts);
    }
}
