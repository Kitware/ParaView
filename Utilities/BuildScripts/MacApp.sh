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
builddir=${srcdir_prefix}/ParaViewAppBin

export DYLD_LIBRARY_PATH=/Users/partyd/Dashboards/Support/qt-4.6.2/bin/lib:/Users/partyd/Kitware/Support/ffmpeg-universal/lib:${builddir}/bin

mkdir ${srcdir_prefix}
cd ${srcdir_prefix}

## Checkout the version requested.
#echo "Checking out version: ${cvstag}"
#cvs -q -d :pserver:anoncvs@www.paraview.org:/cvsroot/ParaView3 co -r ${cvstag} ParaView3 

# Make the binary directory.
mkdir ${builddir}
cd ${builddir}
echo "Reconfiguring and rebuilding in ${builddir}"

export CC=gcc-4.0
export CXX=g++-4.0

rm CMakeCache.txt

cat >> CMakeCache.txt << EOF
CMAKE_BUILD_TYPE:STRING=Release
BUILD_SHARED_LIBS:BOOL=ON
BUILD_TESTING:BOOL=OFF
VTK_USE_RPATH:BOOL=OFF
PARAVIEW_BUILD_QT_GUI:BOOL=ON
PARAVIEW_ENABLE_PYTHON:BOOL=ON
PYTHON_EXECUTABLE:FILEPATH=/System/Library/Frameworks/Python.framework/Versions/2.3/bin/python
PYTHON_INCLUDE_PATH:PATH=/System/Library/Frameworks/Python.framework/Versions/2.3/include/python2.3
PYTHON_LIBRARY:FILEPATH=/System/Library/Frameworks/Python.framework/Versions/2.3/Python
PARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
PYTHON_UTIL_LIBRARY:FILEPATH=
CMAKE_OSX_ARCHITECTURES:STRING=i386;ppc
CMAKE_OSX_SYSROOT:PATH=/Developer/SDKs/MacOSX10.4u.sdk
CMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.4
QT_QMAKE_EXECUTABLE:FILEPATH=/Users/partyd/Dashboards/Support/qt-4.6.2/bin/bin/qmake
VTK_USE_FFMPEG_ENCODER:BOOL=ON
VTK_USE_QVTK_QTOPENGL:BOOL=ON
FFMPEG_INCLUDE_DIR:PATH=/Users/partyd/Kitware/Support/ffmpeg-universal/include
FFMPEG_avcodec_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg-universal/lib/libavcodec.dylib
FFMPEG_avformat_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg-universal/lib/libavformat.dylib
FFMPEG_avutil_LIBRARY:FILEPATH=/Users/partyd/Kitware/Support/ffmpeg-universal/lib/libavutil.dylib
VTK_USE_64BIT_IDS:BOOL=OFF
BUILD_DOCUMENTATION:BOOL=ON
PARAVIEW_GENERATE_PROXY_DOCUMENTATION:BOOL=ON
GENERATE_FILTERS_DOCUMENTATION:BOOL=ON
EOF

cmake ../ParaView

echo "Building Full Package..."
make -j5

echo "Generating package(s) using CPACK"
cpack --config ${builddir}/Applications/ParaView/CPackParaViewConfig.cmake -G DragNDrop 
cd ${builddir}
