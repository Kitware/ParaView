#!/bin/sh

set -e

readonly mindeps_version="3.12.4"
readonly mindeps_prefix="cmake-mindeps"
readonly latest_version="3.21.2"
readonly latest_prefix="cmake"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        mindeps_sha256sum="486edd6710b5250946b4b199406ccbf8f567ef0e23cfe38f7938b8c78a2ffa5f"
        mindeps_platform="Linux-x86_64"
        latest_sha256sum="d5517d949eaa8f10a149ca250e811e1473ee3f6f10935f1f69596a1e184eafc1"
        latest_platform="linux-x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        mindeps_sha256sum="95d76c00ccb9ecb5cb51de137de00965c5e8d34b2cf71556cf8ba40577d1cff3"
        mindeps_platform="Darwin-x86_64"
        latest_sha256sum="25e3f439c19185f51136126a06e14b4873243ea1b4a37678881adde05433ae9b"
        latest_platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly mindeps_sha256sum
readonly mindeps_platform
readonly latest_sha256sum
readonly latest_platform


# Select the CMake version to install
readonly cmake_version="${1:-latest}"

case "$cmake_version" in
    latest)
        version="$latest_version"
        sha256sum="$latest_sha256sum"
        platform="$latest_platform"
        prefix="$latest_prefix"
        ;;
    mindeps)
        version="$mindeps_version"
        sha256sum="$mindeps_sha256sum"
        platform="$mindeps_platform"
        prefix="$mindeps_prefix"

        # Skip if we're not in a `mindeps` job.
        if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'mindeps'; then
            exit 0
        fi
        ;;
    *)
        echo "Unknown CMake version: $cmake_version"
        exit 1
esac
readonly version
readonly sha256sum
readonly platform
readonly prefix

readonly filename="cmake-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" "$prefix"

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin "$prefix/bin"
fi

# Use "latest" version of cmake for spack
if [ "$CI_JOB_NAME" = "build:spack-centos7" ]; then
    mkdir -p "$CI_PROJECT_DIR/build/spack"
    sed \
        -e "s/CMAKE_VERSION/$latest_version/" \
        -e "s,CMAKE_PREFIX,$PWD/cmake," \
        < "$CI_PROJECT_DIR/Utilities/spack/configs/gitlab-ci/packages.yaml.in" \
        > "$CI_PROJECT_DIR/build/spack/packages.yaml"
fi
