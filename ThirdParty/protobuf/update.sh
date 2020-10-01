#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="protobuf"
readonly ownership="protobuf Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/protobuf.git"
readonly tag="for/paraview-20201001-3.13.0"

readonly paths="
CMakeLists.txt
LICENSE
README.kitware.md
README.md
.gitattributes

cmake/protobuf-function.cmake
src/CMakeLists.txt
src/google/protobuf/
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -rvf src/google/protobuf/compiler/csharp
    rm -rvf src/google/protobuf/compiler/java
    rm -rvf src/google/protobuf/compiler/javanano
    rm -rvf src/google/protobuf/compiler/js
    rm -rvf src/google/protobuf/compiler/objectivec
    rm -rvf src/google/protobuf/compiler/php
    rm -rvf src/google/protobuf/compiler/python
    rm -rvf src/google/protobuf/compiler/ruby
    rm -rvf src/google/protobuf/testdata/
    rm -rvf src/google/protobuf/testing/
    rm -rvf src/google/protobuf/util/internal/testdata/
    rm -rvf src/google/protobuf/unittest.proto
    rm -rvf src/google/protobuf/mock_code_generator.cc
    find -name "*.sh" -exec rm -v '{}' \;
    find -name "*_test.*" -exec rm -v '{}' \;
    find -name "*_unittest.*" -exec rm -v '{}' \;
    find -name "*test_*" -exec rm -v '{}' \;
    find -name "*unittest_*" -exec rm -v '{}' \;
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
