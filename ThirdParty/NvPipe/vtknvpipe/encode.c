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
/* Encoding backend that directly uses the "nvEnc" API. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#	include "winposix.h"
#else
#	include <dlfcn.h>
#endif

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <nvToolsExt.h>
#include <nvEncodeAPI.h>
#include "config.nvp.h"
#include "debug.h"
#include "internal-api.h"
#include "nvpipe.h"
#include "yuv.h"

#ifdef _WIN64
static const char* const NVENC_LIB = "nvEncodeAPI64.dll";
#elif defined(_WIN32)
static const char* const NVENC_LIB = "nvEncodeAPI.dll";
#else
static const char* const NVENC_LIB = "libnvidia-encode.so.1";
#endif

DECLARE_CHANNEL(enc);

struct nvp_encoder {
    nvp_impl_t impl;
    uint64_t bitrate;
    NV_ENCODE_API_FUNCTION_LIST f;
    void* lib; /**< dlopen'd library handle for nvenc */
    CUcontext ctx;
    void* encoder; /**< the nvEnc opaque object */
    bool initialized;
    uint32_t width;
    uint32_t height;
    CUdeviceptr rgb; /* GPU copy of input buffer.  Might actually be RGBA. */
    struct {
        size_t pitch;
        CUdeviceptr buf;
        void* registered;
        NV_ENC_OUTPUT_PTR bstream;
    } nv12;
    /* The widely-deployed (on supercomputers) NvEnc 5.x API only supports NV12
     * inputs (more modern versions of the video codec SDK support RGB inputs).
     * To keep things simple, we don't bother with separate paths for newer APIs;
     * we just convert /everything/ to nv12.
     * This "future" implements the conversion.  See yuv.[ch]. */
    nv_fut_t* reorg;
    uint32_t max_width; /* Max encoder width supported by HW+SDK. */
    uint32_t max_height; /* Max encoder height supported by HW+SDK. */
};

#define CLEAR_DL_ERRORS() { \
    const char* errmsg_ = dlerror(); \
    while(errmsg_ != NULL) { \
    WARN(enc, "previous dlerror at %s:%d: %s", __FUNCTION__, __LINE__, \
    errmsg_); \
    errmsg_ = dlerror(); \
    } }

// Mappings from NVENCSTATUS error codes to user-readable error messages
static const char* NvCodecEncErrors[] = {
    "success", /* NV_ENC_SUCCESS */
    "no available encode devices", /* NV_ENC_ERR_NO_ENCODE_DEVICE */
    "available devices do not support encode", /* NV_ENC_ERR_UNSUPPORTED_DEVICE */
    "invalid encoder device", /* NV_ENC_ERR_INVALID_ENCODERDEVICE */
    "invalid device", /* NV_ENC_ERR_INVALID_DEVICE */
    "needs reinitialization", /* NV_ENC_ERR_DEVICE_NOT_EXIST */
    "invalid pointer", /* NV_ENC_ERR_INVALID_PTR */
    "invalid completion event", /* NV_ENC_ERR_INVALID_EVENT */
    "invalid parameter", /* NV_ENC_ERR_INVALID_PARAM */
    "invalid call", /* NV_ENC_ERR_INVALID_CALL */
    "out of memory", /* NV_ENC_ERR_OUT_OF_MEMORY */
    "encoder not initialized", /* NV_ENC_ERR_ENCODER_NOT_INITIALIZED */
    "unsupported parameter", /* NV_ENC_ERR_UNSUPPORTED_PARAM */
    "lock busy (try again)", /* NV_ENC_ERR_LOCK_BUSY */
    "not enough buffer", /* NV_ENC_ERR_NOT_ENOUGH_BUFFER */
    "invalid version", /* NV_ENC_ERR_INVALID_VERSION */
    "map (of input buffer) failed", /* NV_ENC_ERR_MAP_FAILED */
    "need more input (submit more frames!)", /* NV_ENC_ERR_NEED_MORE_INPUT */
    "encoder busy (wait a few ms, call again)", /* NV_ENC_ERR_ENCODER_BUSY */
    "event not registered", /* NV_ENC_ERR_EVENT_NOT_REGISTERD */
    "unknown error", /* NV_ENC_ERR_GENERIC */
    "invalid client key license", /* NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY */
    "unimplemented", /* NV_ENC_ERR_UNIMPLEMENTED */
    "register resource failed", /* NV_ENC_ERR_RESOURCE_REGISTER_FAILED */
    "resource is not registered", /* NV_ENC_ERR_RESOURCE_NOT_REGISTERED */
    "resource not mapped", /* NV_ENC_ERR_RESOURCE_NOT_MAPPED */
};

static const char*
nvcodec_strerror(NVENCSTATUS err) {
    return NvCodecEncErrors[err];
}

/* Marks the end of stream to the encoder.  This forces data to be ready, and
 * theoretically should be done before killing the encoder context.  Sort of a
 * "glFinish" for the encode.
 *
 * NvCodec assumes the user is going to send down N frames and then request frame
 * 0.  H264 (and other codecs) would then compress using its back- and
 * forward-prediction powers.  However, we are going to compress a single frame
 * and then wait for it, meaning forward-prediction is impossible.
 *
 * We don't actually use this except at 'destroy'.  That's because we disable
 * B-frames and immediately ask for every frame after we give it anyway.  If
 * you were doing a bit more asynchronous work, you might use something like
 * this at a significant scene change (i.e. loading a new dataset).  The
 * asynchronicity would net you bonus perf, and flushing ensures you get
 * quality compression without waiting for an I-frame. */
static bool
flush_encoder(struct nvp_encoder* nvp, size_t timestamp) {
    NV_ENC_PIC_PARAMS enc = {0};
    enc.version = NV_ENC_PIC_PARAMS_VER;
    enc.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    enc.inputTimeStamp = timestamp;
    const NVENCSTATUS flsh = nvp->f.nvEncEncodePicture(nvp->encoder, &enc);
    if(flsh != NV_ENC_SUCCESS) {
        ERR(enc, "Error %d flushing frame (nvEncEncodePicture): %s", flsh,
            nvcodec_strerror(flsh));
        return false;
    }
    return true;
}

/* unregister a previously-registered resource.  NvCodec requires one 'register'
 * a chunk of memory before it can be used as an input. */
static void
unregister_resource(struct nvp_encoder* nvp) {
    if(nvp->nv12.registered) {
        void* reg = nvp->nv12.registered;
        if(nvp->f.nvEncUnregisterResource(nvp->encoder, reg) != NV_ENC_SUCCESS) {
            WARN(enc, "Error unregistering nv12 buffer.");
        }
        nvp->nv12.registered = NULL;
    }
}

#define CHECK_CONTEXT(ctx_, errhandling) \
    do { \
    CUcontext current_ = 0x0; \
    const CUresult res_ = cuCtxGetCurrent(&current_); \
    if(CUDA_SUCCESS != res_) { \
    WARN(enc, "Error while checking context: %d", res_); \
    errhandling; \
    } \
    if(ctx_ != current_) { \
    WARN(enc, "Active context (%p) in %s differs from the context that " \
    "was active at creation time (%p)!", current_, __FUNCTION__, \
    ctx_); \
    errhandling; \
    } \
    } while(0)

/* clean up any memory associated with this instance. */
void
nvp_nvenc_destroy(nvpipe* const __restrict cdc) {
    struct nvp_encoder* nvp = (struct nvp_encoder*)cdc;
    assert(nvp->impl.type == ENCODER);

    CHECK_CONTEXT(nvp->ctx, {});

    if(nvp->initialized) {
        flush_encoder(nvp, 0);
    }
    if(nvp->rgb) {
        const cudaError_t merr = cudaFree((void*)nvp->rgb);
        if(merr != cudaSuccess) {
            WARN(enc, "error %d deallocating temporary memory", (int)merr);
        }
    }
    unregister_resource(nvp);
    if(nvp->nv12.buf) {
        const cudaError_t cuerr = cudaFree((void*)nvp->nv12.buf);
        if(cuerr != cudaSuccess) {
            WARN(enc, "error %d deallocating YUV memory", (int)cuerr);
        }
        nvp->nv12.buf = 0;
    }
    if(nvp->nv12.bstream) {
        if(nvp->f.nvEncDestroyBitstreamBuffer(nvp->encoder, nvp->nv12.bstream) !=
                NV_ENC_SUCCESS) {
            WARN(enc, "error destroying bitstream (output) buffer");
        }
    }

    if(nvp->encoder) {
        const NVENCSTATUS nverr = nvp->f.nvEncDestroyEncoder(nvp->encoder);
        if(nverr != NV_ENC_SUCCESS) {
            WARN(enc, "error %d closing encoder (nvEncDestroyEncoder): %s",
                 (int)nverr, nvcodec_strerror(nverr));
        }
        nvp->encoder = NULL;
    }
    if(dlclose(nvp->lib) != 0) {
        WARN(enc, "Error closing NvCodec encode library: %s", dlerror());
    }
    nvp->lib = NULL;

    if(nvp->reorg) {
        nvp->reorg->destroy(nvp->reorg);
        nvp->reorg = NULL;
    }

    nvp->ctx = 0;
}

/* Registers the memory 'mem' with the encoder. */
static bool
register_resource(struct nvp_encoder* nvp, uint32_t width, uint32_t height,
                  CUdeviceptr mem) {
    NV_ENC_REGISTER_RESOURCE resc = {0};
    resc.version = NV_ENC_REGISTER_RESOURCE_VER;
    resc.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    resc.resourceToRegister = (void*)mem;
    /* We only use the nv12 format. */
    resc.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12_PL;
    resc.width = width;
    resc.height = height;
    resc.pitch = nvp->nv12.pitch;

    const NVENCSTATUS nvres = nvp->f.nvEncRegisterResource(nvp->encoder,
                                                           &resc);
    if(NV_ENC_SUCCESS != nvres) {
        ERR(enc, "error %d registering CUDA memory for NvEnc: %s", nvres,
            nvcodec_strerror(nvres));
        return false;
    }
    nvp->nv12.registered = resc.registeredResource;
    return true;
}

#ifdef _MSC_VER
#	define MIN(a,b) (a) < (b) ? (a) : (b)
#	define MAX(a,b) (a) > (b) ? (a) : (b)
#else
#define MIN(a,b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : b; })
#define MAX(a,b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : b; })
#endif

/* Give back the NvPipe hardcoded rate control settings. */
static NV_ENC_RC_PARAMS
nvp_rate_control(const struct nvp_encoder* __restrict nvp) {
    NV_ENC_RC_PARAMS rc = {0};
    rc.version = NV_ENC_RC_PARAMS_VER;
    /* Prefer quality.  s/QUALITY/FRAMESIZE_CAP/ to optimize for size. */
    rc.rateControlMode = NV_ENC_PARAMS_RC_2_PASS_QUALITY;
    /* Protect against overflow. */
    const unsigned RATE_4K_30FPS = 140928614u;
    rc.maxBitRate = MAX(nvp->bitrate*2u, RATE_4K_30FPS);
    rc.averageBitRate = MIN(nvp->bitrate, RATE_4K_30FPS);
#if NVENCAPI_MAJOR_VERSION >= 7
    /* We have lookahead disabled so this setting does not matter, but just to be
     * clear: we do not use B-frames. */
    rc.disableBadapt = 1;
    rc.targetQuality = 51;
    rc.zeroReorderDelay = 1;
    rc.enableNonRefP = 1;
#endif
    return rc;
}

/* Return the NvPipe hardcoded encoder configuration. */
static NV_ENC_CONFIG
nvp_config(const struct nvp_encoder* __restrict nvp) {
    NV_ENC_CONFIG cfg = {0};
    cfg.version = NV_ENC_CONFIG_VER;
    cfg.profileGUID = NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID;
    cfg.gopLength = 15;
    cfg.frameIntervalP = 1; /* use only I and P frames. */
    cfg.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;
    cfg.mvPrecision = NV_ENC_MV_PRECISION_QUARTER_PEL;

    cfg.rcParams = nvp_rate_control(nvp);
    cfg.encodeCodecConfig.h264Config.idrPeriod = (uint32_t)4294967925ULL;
    cfg.encodeCodecConfig.h264Config.adaptiveTransformMode =
            NV_ENC_H264_ADAPTIVE_TRANSFORM_DISABLE;
    cfg.encodeCodecConfig.h264Config.sliceMode = 3;
    cfg.encodeCodecConfig.h264Config.sliceModeData = 4;
    cfg.encodeCodecConfig.h264Config.sliceMode = 0;
    cfg.encodeCodecConfig.h264Config.sliceModeData = 0;
    const uint32_t yuv420 = 1; /* 3 for yuv444 */
    /* Setup an encoder that puts an IDR every 60 frames, an I-frame every 15
     * frames, and the rest P-frames. */
    cfg.encodeCodecConfig.h264Config.chromaFormatIDC = yuv420;
    cfg.encodeCodecConfig.h264Config.maxNumRefFrames = 4;
    cfg.encodeCodecConfig.h264Config.hierarchicalPFrames = 1;
    cfg.encodeCodecConfig.h264Config.enableIntraRefresh = 1;
    cfg.encodeCodecConfig.h264Config.numTemporalLayers = 1;
    cfg.encodeCodecConfig.h264Config.enableConstrainedEncoding = 1;
#if NVENCAPI_MAJOR_VERSION >= 7
    cfg.encodeCodecConfig.h264Config.useConstrainedIntraPred = 1;
#endif
    cfg.encodeCodecConfig.h264Config.level = NV_ENC_LEVEL_AUTOSELECT;
    cfg.encodeCodecConfig.h264Config.enableVFR = 1;
    cfg.encodeCodecConfig.h264Config.idrPeriod = 60;
    cfg.encodeCodecConfig.h264Config.intraRefreshPeriod = 15;
    cfg.encodeCodecConfig.h264Config.intraRefreshCnt = 5;
    cfg.encodeCodecConfig.h264Config.ltrTrustMode = 1;
    cfg.encodeCodecConfig.h264Config.ltrNumFrames = 1;
    return cfg;
}

static bool
initialize(struct nvp_encoder* nvp, uint32_t width, uint32_t height) {
    assert(nvp->initialized == false);
    NV_ENC_CONFIG cfg = nvp_config(nvp);

    NV_ENC_INITIALIZE_PARAMS init = {0};
    init.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init.encodeConfig = &cfg;
    init.encodeGUID = NV_ENC_CODEC_H264_GUID;
    /* 'Latency' here refers to how many frames of delay the encoder will see
     * before we need it to actually spit out a compressed buffer.  30+ frames
     * would be a "normal" latency; we use 0, so that's "low latency".
     * It has little to do with execution time, mostly affects the encoder's
     * use of B-frames. */
    init.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    init.encodeWidth = init.darWidth = width;
    init.encodeHeight = init.darHeight = height;
    init.maxEncodeWidth = nvp->max_width; /* We may resize up to this later. */
    init.maxEncodeHeight = nvp->max_height;
    init.frameRateNum = 30;
    init.frameRateDen = 1;
    /* Async (that is, setting this to 1) is only supported on windows.
     * Regardless, we don't want async because our contract with clients is to be
     * synchronous to allow easy integration into PV/VisIt/VMD/etc. */
    init.enableEncodeAsync = 0;
    init.enablePTD = 1; /* let NvCodec choose between I-frame, P-frame. */
    const NVENCSTATUS nerr = nvp->f.nvEncInitializeEncoder(nvp->encoder, &init);
    if(NV_ENC_SUCCESS != nerr) {
        ERR(enc, "error %d initializing encoder: %s", nerr, nvcodec_strerror(nerr));
        return false;
    }
    return true;
}

/* Create a bitstream buffer.  NvCodec requires one to output into; it can't take
 * just a raw pointer. */
static bool
create_bitstream(struct nvp_encoder* nvp, uint32_t width, uint32_t height,
                 NV_ENC_OUTPUT_PTR* bstream) {
    NV_ENC_CREATE_BITSTREAM_BUFFER bb = {0};
    bb.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
    bb.size = width*height*4;
    bb.memoryHeap = NV_ENC_MEMORY_HEAP_AUTOSELECT;
    bb.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED; /* sample */
    const NVENCSTATUS nvbs = nvp->f.nvEncCreateBitstreamBuffer(nvp->encoder, &bb);
    if(NV_ENC_SUCCESS != nvbs) {
        ERR(enc, "error %d creating output bitstream buffer: %s", (int)nvbs,
            nvcodec_strerror(nvbs));
        return false;
    }
    *bstream = bb.bitstreamBuffer;
    return true;
}

/* Allocates internal RGB[A] and nv12 buffers. */
static bool
nvp_allocate_buffers(struct nvp_encoder* nvp, uint32_t width, uint32_t height) {
    assert(nvp->rgb == 0);
    assert(nvp->nv12.buf == 0);

    const size_t nbytes_rgba = width*height*4;
    const cudaError_t rgberr = cudaMalloc((void**)&nvp->rgb, nbytes_rgba);
    if(cudaSuccess != rgberr) {
        ERR(enc, "error allocating chroma mem: %d", rgberr);
        return false;
    }

    cudaError_t cuerr;
    nvp->nv12.pitch = 0;
    /* NV12 format is width*height elements for Y, (width/2)*(height/2) elements
     * for U, and (width/2)*(height/2) elements for V, thus the height*3/2. */
    cuerr = cudaMallocPitch((void**)&nvp->nv12.buf, &nvp->nv12.pitch, width,
                            height*3/2);
    if(cudaSuccess != cuerr) {
        ERR(enc, "error allocating pitched memory: %d", cuerr);
        cudaFree((void*)nvp->rgb);
        nvp->rgb = 0;
        return false;
    }

    return true;
}

/* There are some aspects that can only be initialized when we know the
 * width/height (which by our API we only get when they call encode),
 * else this would just be inline in the create function. */
static bool
enc_initialize(struct nvp_encoder* nvp, uint32_t width, uint32_t height) {
    if(initialize(nvp, width, height) == false) {
        return false;
    }

    if(!nvp_allocate_buffers(nvp, width, height)) {
        return false;
    }

    if(!create_bitstream(nvp, width, height, &nvp->nv12.bstream)) {
        return false;
    }
    if(!register_resource(nvp, width, height, nvp->nv12.buf)) {
        return false;
    }

    nvp->width = width;
    nvp->height = height;
    nvp->initialized = true;
    return true;
}

static bool
nvp_resize(struct nvp_encoder* nvp, uint32_t width, uint32_t height) {
    assert(nvp->initialized && "Must have previously initialized encoder.");
    /* Don't do anything if we're resizing to the same width/height. */
    if(nvp->width == width && nvp->height == height) {
        return true;
    }
    NV_ENC_CONFIG cfg = nvp_config(nvp);

    NV_ENC_INITIALIZE_PARAMS init = {0};
    init.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init.encodeConfig = &cfg;
    init.encodeGUID = NV_ENC_CODEC_H264_GUID;
    /* 'Latency' here refers to how many frames of delay the encoder will see
     * before we need it to actually spit out a compressed buffer.  30+ frames
     * would be a "normal" latency; we use 0, so that's "low latency".
     * It has nothing to do with performance/time to run, just affects the
     * encoder's use of B-frames. */
    init.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    init.encodeWidth = init.darWidth = width;
    init.encodeHeight = init.darHeight = height;
    init.maxEncodeWidth = nvp->max_width; /* We may resize up to this later. */
    init.maxEncodeHeight = nvp->max_height;
    init.frameRateNum = 30;
    init.frameRateDen = 1;
    /* Async (that is, setting this to 1) is only supported on windows.
     * Regardless, we don't want async because our contract with clients is to be
     * synchronous to allow easy integration into PV/VisIt/etc. */
    init.enableEncodeAsync = 0;
    init.enablePTD = 1;

    NV_ENC_RECONFIGURE_PARAMS rec = {0};
    rec.version = NV_ENC_RECONFIGURE_PARAMS_VER;
    rec.reInitEncodeParams = init;
    rec.resetEncoder = 1;
    const NVENCSTATUS nerr = nvp->f.nvEncReconfigureEncoder(nvp->encoder, &rec);
    if(NV_ENC_SUCCESS != nerr) {
        ERR(enc, "error %d re-initializing: %s", nerr, nvcodec_strerror(nerr));
        return false;
    }

    /* We also need to reallocate our buffers, they're sized to WxH. */
    unregister_resource(nvp);
    if(nvp->f.nvEncDestroyBitstreamBuffer(nvp->encoder, nvp->nv12.bstream) !=
            NV_ENC_SUCCESS) {
        ERR(enc, "error destroying bitstream (output) buffer");
        return false;
    }
    nvp->nv12.bstream = NULL;
    cudaFree((void*)nvp->nv12.buf);
    cudaFree((void*)nvp->rgb);
    nvp->rgb = 0;
    nvp->nv12.buf = 0;
    nvp->nv12.pitch = 0;

    if(!nvp_allocate_buffers(nvp, width, height)) {
        return false;
    }
    if(!create_bitstream(nvp, width, height, &nvp->nv12.bstream)) {
        return false;
    }
    if(!register_resource(nvp, width, height, nvp->nv12.buf)) {
        return false;
    }

    nvp->width = width;
    nvp->height = height;
    return true;
}

/** @return true if the given pointer was allocated on the device. */
static bool
is_device_ptr(const void* ptr) {
    struct cudaPointerAttributes attr;
    const cudaError_t perr = cudaPointerGetAttributes(&attr, ptr);
    return perr == cudaSuccess && attr.devicePointer != NULL;
}

/* Reorganizes input data into NV12 format.
 * The input data must be RGB or RGBA. */
static cudaError_t
reorganize(struct nvp_encoder* nvp, const void* rgb,
           const uint32_t width, const uint32_t height, // padded frame size of device
           const uint32_t widthUser, const uint32_t heightUser, // original input frame size
           const uint32_t ncomponents) {
    assert(ncomponents == 3 || ncomponents == 4);

    /* Create our reorganization object if we need to.  We want to do this first
     * because it houses the stream we'll do our work in, and we might use the
     * stream for a copy. */
    if(nvp->reorg == NULL) {
        nvp->reorg = rgb2nv12(ncomponents);
    }

    /* Where do the data come from?  If it's not a device pointer then we need to
     * copy it to an internal buffer first. */
    const void* src = rgb;
    if(!is_device_ptr(rgb)) {
        const cudaError_t cpyimg =
                cudaMemcpyAsync((void*)nvp->rgb, rgb, widthUser*heightUser*ncomponents,
                                cudaMemcpyHostToDevice, nvp->reorg->strm);
        if(cpyimg != cudaSuccess) {
            ERR(enc, "error copying RGB[A] input buffer to GPU: %d", cpyimg);
            return cpyimg;
        }
        src = (const void*)nvp->rgb;
    }

    cudaError_t org = nvp->reorg->submit(nvp->reorg, (CUdeviceptr)src,
                                         width, height,
                                         widthUser, heightUser,
                                         nvp->nv12.buf, nvp->nv12.pitch);
    if(cudaSuccess != org) {
        return org;
    }

    /* Synchronize immediately.  Semantics of this API are synchronous, even if
     * internal operation is not.  It does not matter much for our case, since
     * we're going to EncodePicture / MapInputResource as soon as we return from
     * this, and neither of those APIs accept stream parameters. */
    org = nvp->reorg->sync(nvp->reorg);
    if(cudaSuccess != org) {
        return org;
    }
    return cudaSuccess;
}

/** encode/compress images
 *
 * User provides pointers for both input and output buffers.  The output buffer
 * must be large enough to accommodate the compressed data.  Sizing the output
 * buffer may be difficult; the call will return OUTPUT_BUFFER_OVERFLOW to
 * indicate that the user must increase the size of the buffer.  The parameter
 * will also be modified to indicate how much of the output buffer was actually
 * used.
 *
 * @param[in] codec library instance to use
 * @param[in] ibuf  input buffer to compress
 * @param[in] ibuf_sz number of bytes in the input buffer
 * @param[out] obuf buffer to place compressed data into
 * @param[in,out] obuf_sz number of bytes available in 'obuf', output is number
 *                        of bytes that were actually filled.
 * @param[in] width number of pixels in X of the input buffer
 * @param[in] height number of pixels in Y of the input buffer
 * @param[in] format the format of ibuf.
 *
 * @return NVPIPE_SUCCESS on success, nonzero on error.
 */
static nvp_err_t
nvp_nvenc_encode(nvpipe * const __restrict codec,
                 const void *const __restrict ibuf,
                 const size_t ibuf_sz,
                 void *const __restrict obuf,
                 size_t* const __restrict obuf_sz,
                 const uint32_t width, const uint32_t height,
                 nvp_fmt_t format) {
    struct nvp_encoder* nvp = (struct nvp_encoder*)codec;
    assert(nvp);
    assert(nvp->impl.type == ENCODER);
    const uint32_t multiplier = format == NVPIPE_RGB ? 3 : 4;
    if(ibuf_sz < sizeof(uint8_t)*multiplier*(size_t)width*(size_t)height) {
        ERR(enc, "Input buffer is too small (%zu bytes) for %ux%u "
                 "RGB[a] image.", ibuf_sz, width, height);
        return NVPIPE_EINVAL;
    }

    CHECK_CONTEXT(nvp->ctx, return NVPIPE_EENCODE);

    /* pad frame size to multiple of 16 */
    uint32_t widthUser = width;
    uint32_t heightUser = height;

    uint32_t widthDevice = width + ((width % 16) != 0 ? 16 - (width % 16) : 0);
    uint32_t heightDevice = height + ((height % 16) != 0 ? 16 - (height % 16) : 0);


    nvp_err_t errcode = NVPIPE_SUCCESS;
    if(!nvp->initialized) {
        if(!enc_initialize(nvp, widthDevice, heightDevice)) {
            ERR(enc, "initialization failed");
            return NVPIPE_EINVAL;
        }
    }
    if(widthDevice != nvp->width || heightDevice != nvp->height) {
        nvp_resize(nvp, widthDevice, heightDevice);
    }

    const cudaError_t rerr = reorganize(nvp, ibuf, widthDevice, heightDevice,
                                        widthUser, heightUser, multiplier);
    if(rerr != cudaSuccess) {
        return rerr;
    }

    /* NvCodec requires one to map the GPU buffer to use it as an encode src. */
    NV_ENC_MAP_INPUT_RESOURCE map = {0};
    map.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    map.registeredResource = nvp->nv12.registered;
    nvtxRangePush("nvenc MapInputResource");
    const NVENCSTATUS mapp = nvp->f.nvEncMapInputResource(nvp->encoder, &map);
    nvtxRangePop();
    if(mapp != NV_ENC_SUCCESS) {
        ERR(enc, "Error %d mapping input: %s", mapp, nvcodec_strerror(mapp));
        return NVPIPE_EMAP;
    }

    NV_ENC_PIC_PARAMS enc = {0};
    enc.version = NV_ENC_PIC_PARAMS_VER;
    enc.inputBuffer = map.mappedResource;
    enc.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12_PL;
    enc.inputWidth = widthDevice;
    enc.inputHeight = heightDevice;
    enc.outputBitstream = nvp->nv12.bstream;
    enc.completionEvent = NULL;
    enc.inputTimeStamp = 0;
    enc.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;

    nvtxRangePush("EncodePicture");
    const NVENCSTATUS encst = nvp->f.nvEncEncodePicture(nvp->encoder, &enc);
    nvtxRangePop();
    if(encst != NV_ENC_SUCCESS) {
        ERR(enc, "Error %d encoding frame: %s", encst, nvcodec_strerror(encst));
        errcode = NVPIPE_EENCODE;
        goto fail_map;
    }

    /* locking the bitstream maps it to host memory; think of it like
     * mapping an OptiX buffer. */
    NV_ENC_LOCK_BITSTREAM bitlock = {0};
    bitlock.version = NV_ENC_LOCK_BITSTREAM_VER;
    bitlock.outputBitstream = nvp->nv12.bstream;
    bitlock.doNotWait = false;
    bitlock.frameIdx = 0;
    nvtxRangePush("nvenc LockBitstream");
    const NVENCSTATUS block = nvp->f.nvEncLockBitstream(nvp->encoder, &bitlock);
    nvtxRangePop();
    if(NV_ENC_SUCCESS != block) {
        ERR(enc, "error %d mapping output: %s", block, nvcodec_strerror(block));
        errcode = NVPIPE_EMAP;
        goto fail_map;
    }
    /* +10: we'll need 10 bytes for the NAL, below. */
    if(bitlock.bitstreamSizeInBytes+10 > *obuf_sz) {
        ERR(enc, "Cannot find 10 bytes in output buffer for NAL.");
        nvp->f.nvEncUnlockBitstream(nvp->encoder, nvp->nv12.bstream);
        errcode = NVPIPE_EOVERFLOW;
        goto fail_map;
    }
    memcpy(obuf, bitlock.bitstreamBufferPtr, bitlock.bitstreamSizeInBytes);

    /* This NAL signals the end of this packet; you might semi-correctly think of
     * this as an h264 frame boundary. */
    uint8_t* nal = ((uint8_t*)obuf) + bitlock.bitstreamSizeInBytes;
    nal[0] = nal[1] = 0;
    nal[2] = 1;
    nal[3] = 9;
    nal[4] = nal[5] = nal[6] = 0;
    nal[7] = 1;
    nal[8] = 9;
    nal[9] = 0;
    *obuf_sz = bitlock.bitstreamSizeInBytes + 10;

    const NVENCSTATUS bunlock = nvp->f.nvEncUnlockBitstream(nvp->encoder,
                                                            nvp->nv12.bstream);
    if(NV_ENC_SUCCESS != bunlock) {
        ERR(enc, "error %d unmapping bitstream: %s", bunlock,
            nvcodec_strerror(bunlock));
        errcode = NVPIPE_EUNMAP;
        goto fail_map;
    }

    NVENCSTATUS umap;
fail_map:
    umap = nvp->f.nvEncUnmapInputResource(nvp->encoder, map.mappedResource);
    if(NV_ENC_SUCCESS != umap) {
        ERR(enc, "Error %d unmapping input: %s previous error: %s", umap,
            nvcodec_strerror(umap), nvcodec_strerror(errcode));
        errcode = NVPIPE_EUNMAP;
    }

    return errcode;
}

/* Asking an encoder to decode makes no sense; this always indicates programmer
 * error.  Bail out. */
nvp_err_t
nvp_nvenc_decode(nvpipe* const __restrict codec,
                 const void* const __restrict ibuf, const size_t ibuf_sz,
                 void* const __restrict obuf,
                 uint32_t width,
                 uint32_t height) {
    (void)codec; (void)ibuf; (void)ibuf_sz;
    (void)obuf;
    (void)width; (void)height;
    ERR(enc, "Encoder does not implement decoding.  Create a decoder.");
    assert(false); /* Such use is always a programmer error. */
    return NVPIPE_EINVAL;
}

static nvp_err_t
nvp_nvenc_bitrate(nvpipe* codec, uint64_t bitrate) {
    struct nvp_encoder* nvp = (struct nvp_encoder*)codec;
    assert(nvp->impl.type == ENCODER);

    CHECK_CONTEXT(nvp->ctx, return NVPIPE_EENCODE);

    nvp->bitrate = bitrate;
    NV_ENC_CONFIG cfg = nvp_config(nvp);

    NV_ENC_INITIALIZE_PARAMS init = {0};
    init.version = NV_ENC_INITIALIZE_PARAMS_VER;
    init.encodeConfig = &cfg;
    init.encodeGUID = NV_ENC_CODEC_H264_GUID;
    /* 'Latency' here refers to how many frames of delay the encoder will see
     * before we need it to actually spit out a compressed buffer.  30+ frames
     * would be a "normal" latency; we use 0, so that's "low latency".
     * It has nothing to do with performance/time to run, just affects the
     * encoder's use of B-frames. */
    init.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    init.encodeWidth = init.darWidth = nvp->width;
    init.encodeHeight = init.darHeight = nvp->height;
    init.maxEncodeWidth = nvp->max_width; /* We may resize up to this later. */
    init.maxEncodeHeight = nvp->max_height;
    init.frameRateNum = 30;
    init.frameRateDen = 1;
    /* Async (that is, setting this to 1) is only supported on windows.
     * Regardless, we don't want async because our contract with clients is to be
     * synchronous to allow easy integration into PV/VisIt/etc. */
    init.enableEncodeAsync = 0;
    init.enablePTD = 1;

    NV_ENC_RECONFIGURE_PARAMS rec = {0};
    rec.version = NV_ENC_RECONFIGURE_PARAMS_VER;
    rec.reInitEncodeParams = init;
    rec.resetEncoder = 0;
    nvp_err_t errcode = NVPIPE_SUCCESS;
    const NVENCSTATUS nerr = nvp->f.nvEncReconfigureEncoder(nvp->encoder, &rec);
    if(NV_ENC_SUCCESS != nerr) {
        ERR(enc, "error %d re-initializing: %s", nerr, nvcodec_strerror(nerr));
        return NVPIPE_EENCODE;
    }

    return errcode;
}

/* The SDK supports multiple possible encoder configurations, denoted by GUIDs.
 * This iterates through the given GUIDs and identifies the max value for the
 * capability given in 'what'. */
static int
max_cap(struct nvp_encoder* nvp, const GUID* guid, uint32_t nguid,
        NV_ENC_CAPS what) {
    NV_ENC_CAPS_PARAM caps = {0};
    caps.version = NV_ENC_CAPS_PARAM_VER;
    caps.capsToQuery = what;

    int max = 0;
    int v = 0;
    for(uint32_t i=0; i < nguid; ++i) {
        caps.capsToQuery = what;
        const NVENCSTATUS cps =
                nvp->f.nvEncGetEncodeCaps(nvp->encoder, guid[i], &caps, &v);
        if(NV_ENC_SUCCESS != cps) {
            WARN(enc, "error %d for encode cap %d, guid %u: %s", cps, what, i,
                 nvcodec_strerror(cps));
        }
        max = MAX(max, v);
    }
    return max;
}

static nvp_err_t
query_max_dimensions(struct nvp_encoder* nvp) {
    const GUID guid[] = {NV_ENC_CODEC_H264_GUID};

    nvp->max_width = (uint32_t)max_cap(nvp, guid, 1, NV_ENC_CAPS_WIDTH_MAX);
    nvp->max_height = (uint32_t)max_cap(nvp, guid, 1, NV_ENC_CAPS_HEIGHT_MAX);
    TRACE(enc, "Encoder supports images up to %ux%u", nvp->max_width,
          nvp->max_height);
    if(nvp->max_width == 0 || nvp->max_height == 0) {
        return NVPIPE_EINVAL;
    }

    return NVPIPE_SUCCESS;
}

nvp_impl_t*
nvp_create_encoder(uint64_t bitrate) {
    struct nvp_encoder* nvp = calloc(1, sizeof(struct nvp_encoder));
    nvp->impl.type = ENCODER;
    nvp->impl.encode = nvp_nvenc_encode;
    nvp->impl.bitrate = nvp_nvenc_bitrate;
    nvp->impl.decode = nvp_nvenc_decode;
    nvp->impl.destroy = nvp_nvenc_destroy;
    nvp->bitrate = bitrate;

    CLEAR_DL_ERRORS();
    nvp->lib = dlopen(NVENC_LIB, RTLD_LAZY|RTLD_LOCAL);
    if(nvp->lib == NULL) {
        ERR(enc, "error loading %s: %s", NVENC_LIB, dlerror());
        free(nvp);
        return NULL;
    }
    typedef NVENCSTATUS (NvEncCreateFqn)(void*);
    NvEncCreateFqn* createNv = dlsym(nvp->lib, "NvEncodeAPICreateInstance");
    memset(&nvp->f, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
    nvp->f.version = NV_ENCODE_API_FUNCTION_LIST_VER;
#if NVENCAPI_MAJOR_VERSION == 5
    nvp->f.version = NVENCAPI_STRUCT_VERSION(NV_ENCODE_API_FUNCTION_LIST, 2);
#endif
    const NVENCSTATUS ierr = createNv(&nvp->f);
    if(NV_ENC_SUCCESS != ierr) {
        ERR(enc, "error %d loading NvCodec encode functions: %s", (int)ierr,
            nvcodec_strerror(ierr));
        dlclose(nvp->lib);
        free(nvp);
        return NULL;
    }

    /* ensure that cuda has created the implicit context. */
    const cudaError_t syncerr = cudaDeviceSynchronize();
    if(cudaSuccess != syncerr) {
        WARN(enc, "Pre-existing CUDA error code: %d", syncerr);
        /* try to continue anyway ... */
    }
    const CUresult currerr = cuCtxGetCurrent(&nvp->ctx);
    if(CUDA_SUCCESS != currerr) {
        ERR(enc, "Error getting context: %d", currerr);
        dlclose(nvp->lib);
        free(nvp);
        return NULL;
    }

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {0};
    /* We have a desire/need to support NvEnc 5.x.  Most people would just want
     * to support whatever version they compiled against, though; in that case,
     * you should use NVENCAPI_VERSION. */
    params.apiVersion = (5 << 4) | 0;
    params.device = nvp->ctx;
    params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    if(nvp->f.nvEncOpenEncodeSessionEx(&params, &nvp->encoder) != NV_ENC_SUCCESS) {
        ERR(enc, "error creating encode session");
        dlclose(nvp->lib);
        free(nvp);
        return NULL;
    }
    assert(nvp->encoder);

    nvp_err_t qry = query_max_dimensions(nvp);
    if(NVPIPE_SUCCESS != qry) {
        ERR(enc, "Error %d querying dimensions: %s", qry, nvpipe_strerror(qry));
        dlclose(nvp->lib);
        free(nvp);
        return NULL;
    }

    // We can't initialize until we know resolution, which only comes on encode.
    nvp->initialized = false;
    return (nvp_impl_t*)nvp;
}
