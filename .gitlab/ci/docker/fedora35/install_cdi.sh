#!/bin/sh

set -e

readonly cdi_repo="https://gitlab.dkrz.de/mpim-sw/libcdi.git"
readonly cdi_commit="cdi-2.0.4"

readonly cdi_root="$HOME/cdi"
readonly cdi_src="$cdi_root/src"
git clone -b "$cdi_commit" "$cdi_repo" "$cdi_src"

dnf install -y --setopt=install_weak_deps=False \
    libtool

cdi_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cd "$cdi_src"
    git worktree add "$cdi_root/$subdir" "$cdi_commit"
    cd "$cdi_root/$subdir"

    ./autogen.sh
    ./configure --prefix=$prefix --with-netcdf=yes "$@"
    make -j$(nproc)
    make install
}

# MPI-less
cdi_build nompi /usr \
    --enable-mpi=no

rm -rf "$cdi_root"

dnf clean all
