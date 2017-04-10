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
#include <nvToolsExt.h>
#include <nvToolsExtCuda.h>
#include "config.nvp.h"
#include "debug.h"
#include "module.h"
#include "yuv.h"

/* The PTX file we will look for. */
static const char* PTXFN = "convert.ptx";

DECLARE_CHANNEL(yuv);

static CUresult
strm_sync(void* f) {
	nv_fut_t* fut = (nv_fut_t*)f;
	const CUresult sy = cuStreamSynchronize(fut->strm);
	if(CUDA_SUCCESS != sy) {
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
	const CUresult del = cuStreamDestroy(fut->strm);
	if(CUDA_SUCCESS != del) {
		WARN(yuv, "Error %d destroying stream.", del);
	}
	fut->strm = 0;
}

static nv_fut_t
strm_create() {
	nv_fut_t rv = {0};
	rv.sync = strm_sync;
	rv.destroy = strm_destroy;

	const CUresult cuerr = cuStreamCreate(&rv.strm, CU_STREAM_NON_BLOCKING);
	if(cuerr != CUDA_SUCCESS) {
		ERR(yuv, "Error %d creating stream.", cuerr);
		return rv;
	}
	return rv;
}

typedef struct rgb_convert {
	nv_fut_t fut;
	ptx_fqn_t fqn;
	size_t components;
} rgb2yuv_t;

static CUresult
rgb2yuv_submit(void* conv, const CUdeviceptr rgb, size_t width, size_t height,
               CUdeviceptr nv12, unsigned pitch) {
	rgb2yuv_t* cnv = (rgb2yuv_t*)conv;
	assert(cnv->fqn.mod != NULL);
	assert(cnv->fqn.func != 0);

	/* NvEnc maxes out at 8k anyway. */
	assert(width <= 8192);
	assert(height <= 8192);
	/* We only support RGB and RGBA data. */
	assert(cnv->components == 3 || cnv->components == 4);

	const void* args[] = {
		(void*)&rgb, &width, &height, &cnv->components, (void*)&nv12, &pitch, 0
	};
	const size_t shmem = 0;
	/* NvEnc can't give us a height that isn't evenly divisible. */
	assert(height%2 == 0);
	const CUresult exec = cuLaunchKernel(cnv->fqn.func, (width/16)+1,(height/2),1,
	                                     16,2,1, shmem, cnv->fut.strm,
	                                     (void**)args, NULL);
	return exec;
}

static void
rgb2yuv_destroy(void* r) {
	if(r == NULL) {
		return;
	}
	rgb2yuv_t* conv = (rgb2yuv_t*)r;
	strm_destroy(&conv->fut);
	module_fqn_destroy(&conv->fqn);
	memset(conv, 0, sizeof(rgb2yuv_t));
	free(conv);
}

static nv_fut_t*
rgb2yuv_create(const char* module, const char* fqnname, size_t components,
               const char* paths[], size_t n) {
	rgb2yuv_t* rv = calloc(1, sizeof(rgb2yuv_t));
	rv->fut = strm_create();
	nvtxNameCuStreamA(rv->fut.strm, "encode");
	rv->fut.submit = rgb2yuv_submit;
	/* Overwrite destructor with ours. */
	rv->fut.destroy = rgb2yuv_destroy;
	rv->components = components;

	rv->fqn = load_module(module, paths, n);
	if(NULL == rv->fqn.mod) {
		rv->fut.destroy(rv);
		return NULL;
	}
	if(cuModuleGetFunction(&rv->fqn.func, rv->fqn.mod, fqnname) != CUDA_SUCCESS) {
		ERR(yuv, "could not load '%s' function from %s.", fqnname, PTXFN);
		rv->fut.destroy(rv);
		return NULL;
	}
	return (nv_fut_t*)rv;
}

typedef struct yuv_convert {
	nv_fut_t fut;
	ptx_fqn_t fqn;
} yuv2rgb_t;

static CUresult
yuv2rgb_submit(void* y, const CUdeviceptr nv12, size_t width, size_t height,
               CUdeviceptr rgb, unsigned pitch) {
	yuv2rgb_t* conv = (yuv2rgb_t*)y;
	assert(conv->fqn.mod);

	const void* args[] = {
		(void*)&nv12, &width, &height, &pitch, (void*)&rgb, 0
	};
	assert(height%2 == 0);
	const CUresult exec = cuLaunchKernel(conv->fqn.func, width/16+1,height/2,1,
	                                     16,2,1, 0,
	                                     conv->fut.strm, (void**)args, NULL);
	return exec;
}

static void
yuv2rgb_destroy(void* y) {
	if(y == NULL) {
		return;
	}
	yuv2rgb_t* conv = (yuv2rgb_t*)y;
	strm_destroy(&conv->fut);
	module_fqn_destroy(&conv->fqn);
	memset(conv, 0, sizeof(yuv2rgb_t));
	free(conv);
}

static nv_fut_t*
yuv2rgb_create(const char* module, const char* fqnname, const char* paths[],
               size_t n) {
	yuv2rgb_t* rv = calloc(1, sizeof(yuv2rgb_t));
	rv->fut = strm_create();
	nvtxNameCuStreamA(rv->fut.strm, "decode");
	rv->fut.submit = yuv2rgb_submit;
	/* Overwrite destructor with ours. */
	rv->fut.destroy = yuv2rgb_destroy;

	rv->fqn = load_module(module, paths, n);
	if(NULL == rv->fqn.mod) {
		rv->fut.destroy(rv);
		return NULL;
	}
	if(cuModuleGetFunction(&rv->fqn.func, rv->fqn.mod, fqnname) != CUDA_SUCCESS) {
		ERR(yuv, "could not load '%s' function from %s.", fqnname, PTXFN);
		cuModuleUnload(rv->fqn.mod);
		rv->fut.destroy(rv);
	}
	return (nv_fut_t*)rv;
}

nv_fut_t*
rgb2nv12(size_t components, const char* paths[], size_t n) {
	return rgb2yuv_create(PTXFN, "rgb2yuv", components, paths, n);
}
nv_fut_t* nv122rgb(const char* paths[], size_t n) {
	return yuv2rgb_create(PTXFN, "yuv2rgb", paths, n);
}
