#!/bin/sh

set -e

# Use commontk pythonqt with cmake support
# CMake support may be added to the official repository in the future. https://github.com/commontk/PythonQt/issues/86
readonly pythonqt_repo="https://github.com/MeVisLab/pythonqt"
readonly pythonqt_commit="v3.3.0"

readonly pythonqt_root="$HOME/pythonqt"
readonly pythonqt_src="$pythonqt_root/src"
readonly pythonqt_build_root="$pythonqt_root/build"

git clone "$pythonqt_repo" "$pythonqt_src"
git -C "$pythonqt_src" checkout "$pythonqt_commit"

# Allow cherry picking.
git -C "$pythonqt_src" config user.name "VTK CI"
git -C "$pythonqt_src" config user.email "kwrobot+vtkci@kitware.com"

# Apply support for 5.15 (PR #64)
git -C "$pythonqt_src" fetch origin refs/pull/64/head
git -C "$pythonqt_src" cherry-pick FETCH_HEAD

pythonqt_build () {
    local prefix="$1"
    shift

    # For python 3.10 compatibility https://github.com/commontk/PythonQt/pull/85
    sed '/#include <pydebug\.h>/d' -i "$pythonqt_src/src/PythonQt.cpp"

    mkdir -p "$pythonqt_build_root"
    pushd "$pythonqt_build_root"
    qmake-qt5 -r "$pythonqt_src/PythonQt.pro" CONFIG+=release \
        QT_SELECT=qt5 \
        PYTHON_VERSION="$( python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")' )"
    make "-j$( nproc )"
    make install
    popd
}

pythonqt_build /usr

rm -rf "$pythonqt_root"
