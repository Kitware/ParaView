# Set up the tools and environment of the Linux jobs. This is sourced (and not
# run) so that the environment changes persist. It is also used by VTK's CI to
# build ParaView with VTK changes, so it does not handle submodules.

.gitlab/ci/cmake.sh
.gitlab/ci/cmake.sh mindeps
.gitlab/ci/ninja.sh
.gitlab/ci/uv.sh
.gitlab/ci/doxygen.sh
.gitlab/ci/sccache.sh
export PATH="$PWD/.gitlab:$PWD/.gitlab/cmake/bin:$PWD/.gitlab/doxygen/bin:$PATH"

# Download Qt
cmake -P .gitlab/ci/download_qt6.cmake
cmake -P .gitlab/ci/download_onnxruntime.cmake
export CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH:$PWD/.gitlab/qt6:$PWD/.gitlab/onnxruntime"

cmake --version
ninja --version
uv --version
doxygen --version

# Load any modules that may be necessary.
if [ -n "$MODULES" ]; then
    . /etc/profile.d/modules.sh
    module load $MODULES
fi

# Enable the Intel compiler if required.
if [ "$CC" = "icx" ]; then
    . /opt/intel/oneapi/setvars.sh || true
fi
