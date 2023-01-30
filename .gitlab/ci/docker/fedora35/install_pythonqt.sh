#!/bin/sh

# Use commontk pythonqt with cmake support
# CMake support may be added to the official repository in the future. https://github.com/commontk/PythonQt/issues/86
readonly pythonqt_repo="https://github.com/commontk/PythonQt"
readonly pythonqt_commit="patched-10"

readonly pythonqt_root="$HOME/pythonqt"
readonly pythonqt_src="$pythonqt_root/src"
readonly pythonqt_build_root="$pythonqt_root/build"

git clone "$pythonqt_repo" "$pythonqt_src"
git -C "$pythonqt_src" checkout "$pythonqt_commit"

pythonqt_build () {
    local prefix="$1"
    shift

    # For python 3.10 compatibility https://github.com/commontk/PythonQt/pull/85
    sed '/#include <pydebug\.h>/d' -i "$pythonqt_src/src/PythonQt.cpp"

    cmake -GNinja \
        -S "$pythonqt_src" \
        -B "$pythonqt_build_root" \
        -DPythonQt_Wrap_Qtcore=ON \
        -DPythonQt_Wrap_Qtgui=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIB_SUFFIX=64 \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$pythonqt_build_root" --target install
}

pythonqt_build /usr

rm -rf "$pythonqt_root"
