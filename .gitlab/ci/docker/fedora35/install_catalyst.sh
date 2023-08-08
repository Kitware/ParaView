#!/bin/sh
set -x

readonly catalyst_repo="https://gitlab.kitware.com/paraview/catalyst"
# we are pre-release, use the most recent commit (Aug 8, 2023)
readonly catalyst_commit="4143e3ca09b568e425f34aa8c3f1b3bd21433a5a"

readonly catalyst_root="$HOME/catalyst"
readonly catalyst_src="$catalyst_root/src"
readonly catalyst_build_root="$catalyst_root/build"

git clone "$catalyst_repo" "$catalyst_src"
git -C "$catalyst_src" checkout "$catalyst_commit"

catalyst_prepare () {
  # make sure we have a recent cmake
  pushd "$catalyst_src"
  sh .gitlab/ci/cmake.sh
  export PATH="$catalyst_src"/.gitlab/cmake/bin:${PATH}
  popd
}
catalyst_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$catalyst_src" \
        -B "$catalyst_build_root/$subdir" \
        -DCATALYST_BUILD_SHARED_LIBS=ON \
        -DCATALYST_BUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCATALYST_WRAP_PYTHON=ON \
        -DCATALYST_WRAP_FORTRAN=ON \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$catalyst_build_root/$subdir" --target install
}

catalyst_prepare

# MPI-less
catalyst_build nompi /usr \
    -DCATALYST_USE_MPI=OFF

# MPICH
catalyst_build mpich /usr/lib64/mpich \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

# OpenMPI
catalyst_build openmpi /usr/lib64/openmpi \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

rm -rf "$catalyst_root"
