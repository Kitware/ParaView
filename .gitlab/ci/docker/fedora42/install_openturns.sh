#!/bin/sh

set -e

readonly openturns_repo="https://github.com/openturns/openturns.git"
readonly openturns_commit="v1.24rc1"

readonly openturns_root="$HOME/openturns"
readonly openturns_src="$openturns_root/src"
readonly openturns_build_root="$openturns_root/build"

git clone -b "$openturns_commit" "$openturns_repo" "$openturns_src"

dnf install -y --setopt=install_weak_deps=False \
   lapack-devel

openturns_build () {
    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$openturns_src" \
        -B "$openturns_build_root" \
        -DBUILD_PYTHON=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DUSE_BISON=OFF \
        -DUSE_BONMIN=OFF \
        -DUSE_BOOST=OFF \
        -DUSE_CERES=OFF \
        -DUSE_CMINPACK=OFF \
        -DUSE_CUBA=OFF \
        -DUSE_CXX17=OFF \
        -DUSE_DLIB=OFF \
        -DUSE_DOXYGEN=OFF \
        -DUSE_FLEX=OFF \
        -DUSE_HDF5=OFF \
        -DUSE_IPOPT=OFF \
        -DUSE_LIBXML2=OFF \
        -DUSE_MPC=OFF \
        -DUSE_MPFR=OFF \
        -DUSE_MUPARSER=OFF \
        -DUSE_NANOFLANN=OFF \
        -DUSE_NLOPT=OFF \
        -DUSE_OPENBLAS=OFF \
        -DUSE_OPENMP=OFF \
        -DUSE_PAGMO=OFF \
        -DUSE_PRIMESIEVE=OFF \
        -DUSE_SPECTRA=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$openturns_build_root/$subdir"
    cmake --build "$openturns_build_root/$subdir" --target install
}

openturns_build /usr

cd

rm -rf "$openturns_root"

dnf clean all
