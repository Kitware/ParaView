/* Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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
#define _POSIX_C_SOURCE 201212L
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nvToolsExt.h>
#include "debug.h"
#include "module.h"
#include "config.nvp.h"

DECLARE_CHANNEL(module)

static unsigned long
path_max() {
	errno = 0;
	long rv = pathconf("/", _PC_PATH_MAX);
	if(rv == -1 && errno != 0) {
		ERR(module, "Could not lookup path max for /: %ld", rv);
		return 0;
	} else if(rv == -1) {
		rv = 4096;
	}
	return (unsigned long)rv;
}

/* Finding a PTX module for an installed library is painful.  This searches
 * some standard places until it finds one. */
ptx_fqn_t
load_module(const char* module, const char* paths[], const size_t n) {
	ptx_fqn_t rv = {0};

	CUresult ld = CUDA_ERROR_FILE_NOT_FOUND;
	const size_t pathlen = path_max();
	char* fname = calloc(pathlen+1,1);

	const char* userpath = getenv("NVPIPE_PTX");
	nvtxRangePush("load CUDA module");
	if(userpath) {
		strncpy(fname, userpath, pathlen);
		strncat(fname, "/", pathlen);
		strncat(fname, module, pathlen-strlen(userpath)-1);
		ld = cuModuleLoad(&rv.mod, fname);
	} else {
		for(size_t i=0; i < n; ++i) {
			strncpy(fname, paths[i], pathlen);
			const char* post = "/share/nvpipe/";
			strncat(fname, post, pathlen);
			strncat(fname, module, pathlen-strlen(paths[i])-strlen(post));
			ld = cuModuleLoad(&rv.mod, fname);
			if(ld == CUDA_SUCCESS) {
				break;
			}
			WARN(module, "Could not load '%s': %d", fname, ld);
			strncpy(fname, paths[i], pathlen);
			strncat(fname, module, pathlen-strlen(paths[i]));
			ld = cuModuleLoad(&rv.mod, fname);
			if(ld == CUDA_SUCCESS) {
				break;
			}
			WARN(module, "Could not load '%s': %d", fname, ld);
		}
	}
	nvtxRangePop();
	free(fname);
	if(CUDA_SUCCESS != ld) {
		ERR(module, "error loading %s: %d", module, ld);
		rv.mod = NULL;
	}
	return rv;
}

void
module_fqn_destroy(ptx_fqn_t* cnv) {
	if(cnv == NULL) {
		return;
	}
	if(cnv->mod != NULL) {
		const CUresult cuerr = cuModuleUnload(cnv->mod);
		if(CUDA_SUCCESS != cuerr) {
			WARN(module, "Error %d unloading conversion module.", cuerr);
		}
		cnv->mod = NULL;
	}
	cnv->func = NULL;
}

/** Setup initial module paths.  All paths are dynamically allocated, and the
 * array is dynamic as well; the caller should free.
 * @param[out] n the number of paths setup. */
char**
module_paths(size_t* n) {
	const size_t N_PREFIX = 4;
	char** rv = malloc(N_PREFIX*sizeof(char*));
	/* Copy everything even when unneeded; makes it easier at destroy() time. */
	rv[0] = strdup(NVPIPE_PREFIX);
	rv[1] = strdup(".");
	rv[2] = strdup("/usr");
	rv[3] = strdup("/usr/local");
	*n = N_PREFIX;
	return rv;
}
