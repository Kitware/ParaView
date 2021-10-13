#!/bin/sh

set -e

readonly version="3.21.2"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="d5517d949eaa8f10a149ca250e811e1473ee3f6f10935f1f69596a1e184eafc1"
        platform="linux-x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="25e3f439c19185f51136126a06e14b4873243ea1b4a37678881adde05433ae9b"
        platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="cmake-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" cmake

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin cmake/bin
fi

if [ "$CI_JOB_NAME" = "build:spack-centos7" ]; then
    mkdir -p "$CI_PROJECT_DIR/build/spack"
    sed \
        -e "s/CMAKE_VERSION/$version/" \
        -e "s,CMAKE_PREFIX,$PWD/cmake," \
        < "$CI_PROJECT_DIR/Utilities/spack/configs/gitlab-ci/packages.yaml.in" \
        > "$CI_PROJECT_DIR/build/spack/packages.yaml"
fi
