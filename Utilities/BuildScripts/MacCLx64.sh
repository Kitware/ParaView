#!/bin/bash


cd ../..
srcdir=${PWD}
cd ..
rootdir=${PWD}
builddir=${rootdir}/ParaViewCmdBin

export DYLD_LIBRARY_PATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib:${builddir}/bin

# Make the binary directory.
mkdir ${builddir}
cd ${builddir}
echo "Reconfiguring and rebuilding in ${builddir}"

rm CMakeCache.txt

cat >> CMakeCache.txt << EOF
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_CXX_FLAGS_RELEASE:STRING=-O2 -DNDEBUG
CMAKE_C_FLAGS_RELEASE:STRING=-O2 -DNDEBUG
BUILD_SHARED_LIBS:BOOL=ON
BUILD_TESTING:BOOL=OFF
VTK_USE_RPATH:BOOL=OFF
CMAKE_OSX_ARCHITECTURES:STRING=x86_64
CMAKE_OSX_SYSROOT:PATH=/Developer/SDKs/MacOSX10.5.sdk
CMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.5
PARAVIEW_BUILD_QT_GUI:BOOL=OFF
PARAVIEW_ENABLE_PYTHON:BOOL=ON
PYTHON_EXECUTABLE:FILEPATH=/System/Library/Frameworks/Python.framework/Versions/2.5/bin/python
PYTHON_INCLUDE_PATH:PATH=/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5
PYTHON_LIBRARY:FILEPATH=/System/Library/Frameworks/Python.framework/Versions/2.5/Python
PARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
PYTHON_UTIL_LIBRARY:FILEPATH=
PARAVIEW_BUILD_PLUGIN_PointSprite:BOOL=OFF
PARAVIEW_BUILD_PLUGIN_PrismPlugins:BOOL=OFF
PARAVIEW_BUILD_PLUGIN_Moments:BOOL=OFF
PARAVIEW_BUILD_PLUGIN_pvblot:BOOL=OFF
VTK_USE_FFMPEG_ENCODER:BOOL=ON
FFMPEG_INCLUDE_DIR:PATH=/Users/partyd/Kitware/Support/ffmpeg/bin/include
FFMPEG_avcodec_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavcodec.dylib
FFMPEG_avformat_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavformat.dylib
FFMPEG_avutil_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavutil.dylib
FFMPEG_swscale_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libswscale.dylib
VTK_USE_64BIT_IDS:BOOL=OFF
EOF

cmake ${srcdir}

echo "Building Command Line Tools..."
make -j18

echo "Generating package(s) using CPACK"
cpack --config ${builddir}/Servers/Executables/CPackParaViewServersConfig.cmake -G TGZ
cd ${builddir}

