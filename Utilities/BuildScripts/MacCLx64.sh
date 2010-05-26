#!/bin/bash
# Typical usage:
#  ~/Kitware/buildPackage.sh /tmp/Kitware/ParaView3/ /tmp/Kitware/ParaView3Bin
#  ~/Kitware/buildParaViewPackage.sh <version> <cvstag>

if [ "$#" != "1" ]; then
  echo "Usage: $0 <cvstag>"
  exit 1
fi


cvstag=$1

srcdir_prefix=${PWD}
srcdir=${srcdir_prefix}/ParaView
builddir=${srcdir_prefix}/ParaViewCmdBinx64

export DYLD_LIBRARY_PATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib:${builddir}/bin

#mkdir ${srcdir_prefix}
cd ${srcdir_prefix}

## Checkout the version requested.
#echo "Checking out version: ${cvstag}"
#cvs -q -d :pserver:anoncvs@www.paraview.org:/cvsroot/ParaView3 co -r ${cvstag} ParaView3 

# Make the binary directory.
mkdir ${builddir}
cd ${builddir}
echo "Reconfiguring and rebuilding in ${builddir}"

rm CMakeCache.txt

cat >> CMakeCache.txt << EOF
CMAKE_BUILD_TYPE:STRING=Release
BUILD_SHARED_LIBS:BOOL=ON
BUILD_TESTING:BOOL=OFF
VTK_USE_RPATH:BOOL=OFF
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
CMAKE_OSX_ARCHITECTURES:STRING=x86_64
CMAKE_OSX_SYSROOT:PATH=/Developer/SDKs/MacOSX10.5.sdk
CMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.5
VTK_USE_FFMPEG_ENCODER:BOOL=ON
FFMPEG_INCLUDE_DIR:PATH=/Users/partyd/Kitware/Support/ffmpeg/bin/include
FFMPEG_avcodec_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavcodec.dylib
FFMPEG_avformat_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavformat.dylib
FFMPEG_avutil_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libavutil.dylib
FFMPEG_swscale_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg/bin/lib/libswscale.dylib
PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin:BOOL=OFF
VTK_USE_64BIT_IDS:BOOL=OFF
EOF

cmake ../ParaView

echo "Building Command Line Tools..."
make -j5

echo "Generating package(s) using CPACK"
cpack --config ${builddir}/Servers/Executables/CPackParaViewServersConfig.cmake -G TGZ
cd ${builddir}

