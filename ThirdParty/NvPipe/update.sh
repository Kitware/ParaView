#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="NvPipe"
readonly ownership="nvpipe Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/${name}/vtknvpipe"
readonly repo="https://gitlab.kitware.com/third-party/nvpipe.git"
readonly tag="for/paraview"

readonly paths="
.gitattributes
CMakeLists.txt
README.md
README.kitware.md

api.c
CMakeLists.txt
config.nvp.h.in
convert.cu
debug.c
debug.h
decode.c
encode.c
error.c
internal-api.h
LICENSE
mangle_nvpipe.h
nvEncodeAPI-v5.h
nvpipe.h
winposix.c
winposix.h
yuv.c
yuv.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
