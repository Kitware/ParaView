#!/bin/sh

set -e

readonly version="3.17.4"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="126cc8356907913787d4ff35237ae1854c09b927a35dbe5270dd571ae224bdd3"
        platform="Linux"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="125eaf2befeb8099237298424c6e382b40cb23353ee26ce96545db29ed899b4a"
        platform="Darwin"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="cmake-$version-$platform-x86_64"
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
