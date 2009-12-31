set -x
#!/bin/bash
# Typical usage:
#  ~/partyd/buildPackage.sh /tmp/partyd/ParaView3/ /tmp/partyd/ParaView3Bin
#  ~/partyd/buildParaViewPackage.sh <version> <cvstag>

if [ "$#" != "2" ]; then
  echo "Usage: $0 <version> <cvstag>"
  exit 1
fi

version=$1
cvstag=$2

#sudo apt-get install libglib-dev
#sudo apt-get install cvs
#sudo apt-get install hg
#sudo apt-get install gfortran
#sudo apt-get install libjpeg-dev
#sudo apt-get install flex
#sudo apt-get install bison
#sudo apt-get install g77
#sudo apt-get install tcl8.4-dev
#sudo apt-get install tk8.4-dev
#sudo apt-get install libglut3
#sudo apt-get install libglut3-dev
#sudo apt-get install libglib2.0-dev

export FC=gfortran

BASE_DIR=${PWD}
SUPPORT_DIR=${BASE_DIR}/Support
CORES=3

PV_BASE=${BASE_DIR}/ParaView-${version}
PV_SRC=${PV_BASE}/ParaView3
PV_BIN=${PV_BASE}/ParaView3Bin

if [ ! -d Support ];
then
  mkdir Support
fi

cd Support

# QT pre
#sudo apt-get install libpng12-dev
#sudo apt-get install libpng-dev
#sudo apt-get install libfontconfig1-dev


# QT
if [ ! -f $SUPPORT_DIR/qt-4.3.5/bin/bin/qmake ];
then
  wget http://get.qt.nokia.com/qt/source/qt-x11-opensource-src-4.3.5.tar.gz
  tar -zxvf qt-x11-opensource-src-4.3.5.tar.gz
  mkdir qt-4.3.5
  mkdir qt-4.3.5/bin
  mv qt-x11-opensource-src-4.3.5 src
  mv src/ qt-4.3.5/src

  cd qt-4.3.5/src/
  echo yes | ./configure --prefix=${SUPPORT_DIR}/qt-4.3.5/bin/ -opengl -optimized-qmake
  make -j${CORES}
  make install

  cd ../..
else
  echo "QT Complete"
fi

# Python
if [ ! -f ${SUPPORT_DIR}/python25/lib/libpython2.5.so ];
then
  wget http://www.python.org/ftp/python/2.5.4/Python-2.5.4.tgz
  tar -zxvf Python-2.5.4.tgz
  mkidr python25
  cd Python-2.5.4/
  ./configure --prefix=${SUPPORT_DIR}/python25 --enable-shared
  make -j${CORES}
  make install
  cd ..
else
  echo "Python Complete"
fi

# set the LD_LIBRARY_PATH to our built python so sip/PyQt won't mistakenly use the system one.
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SUPPORT_DIR}/python25/lib

# sip
if [ ! -f ${SUPPORT_DIR}/python25/bin/sip ];
then
  wget http://www.riverbankcomputing.co.uk/static/Downloads/sip4/sip-4.9.3.tar.gz
  tar -zxvf sip-4.9.3.tar.gz
  cd sip-4.9.3/
  ${SUPPORT_DIR}/python25/bin/python configure.py
  make -j${CORES}
  make install
  cd ..
else
  echo "SIP Complete"
fi

# PyQt
if [ ! -f ${SUPPORT_DIR}/python25/lib/python2.5/site-packages/PyQt4/QtCore.so ];
then
  wget http://www.riverbankcomputing.co.uk/static/Downloads/PyQt4/PyQt-x11-gpl-4.6.2.tar.gz
  tar -zxvf PyQt-x11-gpl-4.6.2.tar.gz
  cd PyQt-x11-gpl-4.6.2/
  echo yes | ${SUPPORT_DIR}/python25/bin/python configure.py -q ${SUPPORT_DIR}/qt-4.3.5/bin/bin/qmake
  make -j${CORES}
  make install
  cd ..
else
  echo "PyQt Complete"
fi

# CMake
# ./configure --qt-gui --qt-qmake=${SUPPORT_DIR}/qt-4.3.5/bin/bin/qmake
# make -j${CORES}
# sudo make install

# VisIt

if [ ! -f ${SUPPORT_DIR}/VisIt-1.10.0.X-all.tar.gz ];
then
  wget videonas2/paraview/tools/VisIt-1.10.0.X-all.tar.gz
  tar -zxvf VisIt-1.10.0.X-all.tar.gz
fi

cd VisIt-1.10.0.X-all/

# szip
if [ ! -f ${SUPPORT_DIR}/szip-2.1/lib/libsz.a ];
then
  tar -zxvf szip-2.1.tar.gz
  cd szip-2.1/
  LIBS=-lm CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/szip-2.1 --disable-shared
  make -j${CORES}
  make install
  cd ..
else
  echo "SZip Complete"
fi

# HDF4
if [ ! -f ${SUPPORT_DIR}/hdf4-4.2r4/lib/libmfhdf.a ];
then
  tar -zxvf HDF4.2r3.tar.gz
  cd HDF4.2r3/
  LIBS=-lm CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/hdf4-4.2r4 --disable-fortran --with-szlib=${SUPPORT_DIR}/szip-2.1/
  make -j 8
  make install
  cd ..
else
  echo "HDF4 Complete"
fi

# HDF5
# for gcc 4.3 you'll have to edit perform/zip_perf.c change line 549 to "output = open(filename, O_RDWR | O_CREAT, S_IRWXU);"
#
# When you configure your ParaView build do this:
# USE_SYSTEM_HDF5   ON
# HDF5_LIBRARY      ${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/libhdf5.a;${SUPPORT_DIR}/szip-2.1/lib/libsz.a
#
if [ ! -f ${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/libhdf5.a ];
then
  tar -zxvf hdf5-1.6.8.tar.gz
  cd hdf5-1.6.8
  CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/hdf5-1.6.8_ser --disable-shared --disable-fortran --disable-parallel --with-szlib=${SUPPORT_DIR}/szip-2.1 
  make -j${CORES}
  make install
  ln -s ${SUPPORT_DIR}/szip-2.1/lib/libsz.a ${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/libsz.a
  cd ..
else
  echo "HDF5 Complete"
fi

# BoxLib
if [ ! -f ${SUPPORT_DIR}/boxlib/lib/libbox2D.a ];
then
  tar -zxvf boxlib.tar.gz
  cd CCSEApps/BoxLib/
  # For gcc-4.3 add #include <cstdlib> to ParallelDescriptor.cpp
  # Usiing gfortran instead of g77 works well.
  #
  #
  # cd into boxlib directory in the CCSEApps
  # edit GNUMakefile, set USE_MPI=false, COMP=g++

  mv std std.old
  chmod 644 *.H

  (
  cat <<EOF
--- GNUmakefile 2001-07-23 00:32:20.000000000 -0400
+++ GNUmakefile.org     2009-12-29 12:06:07.000000000 -0500
@@ -9,8 +9,8 @@
 PRECISION = DOUBLE
 DEBUG     = TRUE
 DIM       = 3
-COMP      = KCC
-USE_MPI   = TRUE
+COMP      = g++
+USE_MPI   = FALSE
 #NAMESPACE = TRUE
 NAMESPACE = FALSE
EOF
) | patch -p0 -N

  CXXFLAGS=-fPIC CFLAGS=-fPIC FFLAGS=-fPIC make -j${CORES}
  mkdir -p ${SUPPORT_DIR}/boxlib/{include/2D,include/3D,lib}
  cp libbox3d.Linux.g++.f77.DEBUG.a ${SUPPORT_DIR}/boxlib/lib/libbox3D.a
  cp *.H ${SUPPORT_DIR}/boxlib/include/3D/

  # edit GNUMakefile,set DIM=2
  (
  cat <<EOF
--- GNUmakefile 2009-12-29 12:11:06.000000000 -0500
+++ GNUmakefile.org     2009-12-29 12:12:39.000000000 -0500
@@ -8,7 +8,7 @@

 PRECISION = DOUBLE
 DEBUG     = TRUE
-DIM       = 3
+DIM       = 2
 COMP      = g++
 USE_MPI   = FALSE
 #NAMESPACE = TRUE
EOF
) | patch -p0 -N

  
  CXXFLAGS=-fPIC CFLAGS=-fPIC FFLAGS=-fPIC make -j 8
  cp libbox2d.Linux.g++.f77.DEBUG.a ${SUPPORT_DIR}/boxlib/lib/libbox2D.a
  cp *.H ${SUPPORT_DIR}/boxlib/include/2D/
  cd ../..
else
  echo "BoxLib Complete"
fi

# NetCDF
# For gcc-4.3 add #include <cstring> to ./cxx/ncvalues.cpp
if [ ! -f ${SUPPORT_DIR}/netcdf-3.6.0-p1/lib/libnetcdf.a ];
then
  tar -zxvf netcdf.tar.gz

(
cat <<EOF
*** netcdf-3.6.0-p1/src/cxx/ncvalues.cpp.orig   2009-12-18 14:25:10.000000000 -0500
--- netcdf-3.6.0-p1/src/cxx/ncvalues.cpp        2009-12-18 14:25:45.000000000 -0500
***************
*** 8,13 ****
--- 8,14 ----
   *********************************************************************/

  #include <iostream>
+ #include <cstring>

  #include "ncvalues.h"
EOF
) | patch -p0 -N
 
  cd netcdf-3.6.0-p1/src/
  CXXFLAGS=-fPIC CFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/netcdf-3.6.0-p1
  make -j${CORES}
  mkdir ${SUPPORT_DIR}/netcdf-3.6.0-p1
  make install 
  cd ..
else
  echo "NetCDF Complete"
fi

# Silo
# 4.6.2 doesn't work with VisIt and gcc 4.3 as of this writing(2009-02-25). Some sort of link issue, may be libtool.?
if [ ! -f ${SUPPORT_DIR}/silo-4.6.2/lib/libsilo.a ];
then
  tar -zxvf silo-4.6.2.tar.gz
  cd silo-4.6.2/
  ./configure --prefix=${SUPPORT_DIR}/silo-4.6.2 --without-readline --with-hdf5=${SUPPORT_DIR}/hdf5-1.6.8_ser/include/,${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/ --without-exodus --with-szlib=${SUPPORT_DIR}/szip-2.1 --disable-fortran --disable-browser --disable-shared --disable-silex
  make -j${CORES}
  make install
  ln -s ${SUPPORT_DIR}/silo-4.6.2/lib/libsiloh5.a ${SUPPORT_DIR}/silo-4.6.2/lib/libsilo.a
  cd ..
else
  echo "Silo Complete"
fi

# CGNS
# Only use hdf5 if you need it. & you don't! --with-hdf5=${SUPPORT_DIR}/hdf5-1.6.8_ser/
if [ ! -f ${SUPPORT_DIR}/cgns-2.4/lib/libcgns.a ];
then
  tar -zxvf cgnslib_2.4-3.tar.gz
  cd cgnslib_2.4/
  CXXFLAGS=-fPIC CFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/cgns-2.4 --with-szip=${SUPPORT_DIR}/szip-2.1/lib/libsz.a 
  make -j${CORES}
  mkdir -p ${SUPPORT_DIR}/cgns-2.4/{include,lib}
  make install
  cd ..
else
  echo "CGNS Complete"
fi

# CFITSIO
if [ ! -f ${SUPPORT_DIR}/cfitsio/lib/libcfitsio.a ];
then
  tar -zxvf cfitsio3006.tar.gz
  cd cfitsio/
  ./configure --prefix=${SUPPORT_DIR}/cfitsio
  make -j${CORES}
  make install
  cd ..
else
  echo "CFITSIO Complete"
fi

# H5Part
if [ ! -f ${SUPPORT_DIR}/h5part-1.3.3/lib/libH5Part.a ];
then
  tar -zxvf H5Part-20070711.tar.gz
  cd H5Part-1.3.3/
  CFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/h5part-1.3.3 --with-hdf5path=${SUPPORT_DIR}/hdf5-1.6.8_ser/
  make -j${CORES}
  make install
  cd ..
else
  echo "H5Part Complete"
fi

# CCMIO
#=======
# Didn't work & looks like it uses qmake ?!?. If some one complains we'll get it working.
#

# GDAL
#=======
# For gcc-4.3 download gdal-1.6.0, the following config works with both version
#
if [ ! -f ${SUPPORT_DIR}/gdal-1.6.0/lib/libgdal.a ];
then
  tar -zxvf gdal-1.6.0.tar.gz
  cd gdal-1.6.0/
  CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=${SUPPORT_DIR}/gdal-1.6.0 --enable-static --disable-shared --with-libtiff=internal --with-gif=internal --with-png=internal --with-jpeg=internal --with-libz=internal --with-netcdf=no --without-jasper --without-python
  make -j${CORES}
  make install
  cd ..
else
  echo "GDAL Complete"
fi

# Qt 3
#=======
# We don't have to link against Qt but VisIt plugin system requires Qt to build.
if [ ! -f ${SUPPORT_DIR}/qt-3.3.8/bin/qmake ];
then
  tar -zxvf qt-x11-free-3.3.8.tar.gz
  cd qt-x11-free-3.3.8/
  export QTDIR=`pwd`
  export LD_LIBRARY_PATH=$QTDIR/lib
  echo yes | ./configure --prefix=${SUPPORT_DIR}/qt-3.3.8
  make -j${CORES}
  make install
  cd ..
else
  echo "Qt 3.3.8 Complete"
fi

# MPICH
if [ ! -f ${SUPPORT_DIR}/mpich2-1.0.8/lib/libmpich.a ];
then
  tar -zxvf mpich2-1.0.8.tar.gz
  cd mpich2-1.0.8/
  ./configure --prefix=${SUPPORT_DIR}/mpich2-1.0.8/
  make
  make install
  cd ..
else
  echo "MPICH Complete"
fi

# FFMPEG
if [ ! -f ${SUPPORT_DIR}/ffmpeg/lib/libavcodec.so ];
then
  wget http://www.vtk.org/files/support/ffmpeg_source.tar.gz
  tar -zxvf ffmpeg_source.tar.gz
  cd ffmpeg_source
  tar -zxvf ffmpeg.tar.gz
  cd ffmpeg
  ./configure --disable-vhook --disable-static --disable-network --disable-zlib --disable-ffserver --disable-ffplay --disable-decoders --enable-shared --prefix=/${SUPPORT_DIR}/ffmpeg/
  make -j${CORES}
  make install
  cd ../..
else
  echo "FFMPEG Complete"
fi

# ParaView (1st pass)
# Visit needs a build of VTK so we do a first pass build of ParaView
# the final pass will then just be an incremental build.

if [ ! -f ${PV_BIN}/bin/paraview ];
then

if [ ! -d ${PV_BASE} ];
then
  mkdir ${PV_BASE}
fi

cd ${PV_BASE}

## Checkout the version requested.
echo "Checking out version: ${cvstag}"
cvs -q -d :pserver:anoncvs@www.paraview.org:/cvsroot/ParaView3 co -r ${cvstag} ParaView3

if [ ! -d ${PV_SRC}/Plugins/VisTrails ];
then
  cd ${PV_SRC}/Plugins
  hg clone http://blight.kitwarein.com/VisTrails
# edit the CMaklists file to turn on VisTrails
# edit the CMaklists file to turn on VisTrails
(
cat <<EOF
--- CMakeLists.txt	2009-12-30 12:07:09.000000000 -0500
+++ CMakeLists.txt.org	2009-12-30 12:05:36.000000000 -0500
@@ -49,5 +49,6 @@
 paraview_build_optional_plugin(ThresholdTablePanel "ThresholdTablePanel" ThresholdTablePanel OFF) 
 paraview_build_optional_plugin(ClientGraphViewFrame "ClientGraphViewFrame" ClientGraphViewFrame OFF)
 paraview_build_optional_plugin(VisItReaderPlugin "VisItReaderPlugin" VisItDatabaseBridge OFF) 
+paraview_build_optional_plugin(VisTrailsPlugin "VisTrailsPlugin" VisTrails ON)
 paraview_build_optional_plugin(H5PartReader "Reader for *.h5part files" H5PartReader ON)
 
EOF
) | patch -p0 -N
  cd ../..
fi

# Make the binary directory.
if [ ! -d ${PV_BIN} ];
then
  mkdir ${PV_BIN}
fi

cd ${PV_BIN}
echo "Reconfiguring and rebuilding in ${builddir}"

export LD_LIBRARY_PATH=${SUPPORT_DIR}/qt-4.3.5/bin/lib:${SUPPORT_DIR}/ffmpeg/lib:${SUPPORT_DIR}/python25/lib

rm CMakeCache.txt

cat >> CMakeCache.txt << EOF
CMAKE_BUILD_TYPE:STRING=Release
BUILD_SHARED_LIBS:BOOL=ON
VTK_USE_RPATH:BOOL=OFF
PARAVIEW_BUILD_QT_GUI:BOOL=ON
QT_QMAKE_EXECUTABLE:FILEPATH=${SUPPORT_DIR}/qt-4.3.5/bin/bin/qmake
VTK_USE_QVTK_QTOPENGL:BOOL=ON
VTK_USE_64BIT_IDS:BOOL=OFF
PARAVIEW_USE_SYSTEM_HDF5:BOOL=ON
HDF5_LIBRARY:PATH=${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/libhdf5.a;${SUPPORT_DIR}/szip-2.1/lib/libsz.a
HDF5_INCLUDE_DIR:PATH=${SUPPORT_DIR}/hdf5-1.6.8_ser/include
PARAVIEW_BUILD_PLUGIN_VisItReaderPlugin:BOOL=OFF
VISIT_BASE:PATH=${SUPPORT_DIR}/VisIt-1.10.0.X-all/VisItDev1.10.0.X
VTK_USE_FFMPEG_ENCODER:BOOL=ON
FFMPEG_INCLUDE_DIR:PATH=${SUPPORT_DIR}/ffmpeg/include/
FFMPEG_avcodec_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavcodec.so
FFMPEG_avformat_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavformat.so
FFMPEG_avutil_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavutil.so
PARAVIEW_ENABLE_PYTHON:BOOL=ON
PARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
PYTHON_EXECUTABLE:PATH=${SUPPORT_DIR}/python25/bin/python
PYTHON_INCLUDE_PATH:PATH=${SUPPORT_DIR}/python25/include/python2.5/
PYTHON_LIBRARY:PATH=${SUPPORT_DIR}/python25/lib/libpython2.5.so
PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin:BOOL=ON
EOF

cmake ${PV_SRC}
make -j${CORES}

else
  echo "Found paraview"
fi

# Visit
#=======
# write out config file

if [ ! -f ${SUPPORT_DIR}/VisIt-1.10.0.X-all/VisItDev1.10.0.X/src/lib/libplugin.so ];
then

cd ${SUPPORT_DIR}/VisIt-1.10.0.X-all
rm vtkVisitDatabaseBridge.conf

cat >> vtkVisitDatabaseBridge.conf << EOF
#==============================================================================(Configuration)
VTK=\$VTK_BUILD/VTK 
VTK_SOURCE=${PV_SRC}/VTK
VTK_BUILD=${PV_BIN}
VISIT_VTK_CPPFLAGS="\\
      -I\$VTK_SOURCE/Parallel\\
      -I\$VTK_SOURCE/GenericFiltering\\
      -I\$VTK_SOURCE/Views\\
      -I\$VTK_SOURCE/Imaging\\
      -I\$VTK_SOURCE/GUISupport\\
      -I\$VTK_SOURCE/Infovis\\
      -I\$VTK_SOURCE/Hybrid\\
      -I\$VTK_SOURCE/VolumeRendering\\
      -I\$VTK_SOURCE/Examples\\
      -I\$VTK_SOURCE/Wrapping\\
      -I\$VTK_SOURCE/IO\\
      -I\$VTK_SOURCE/Filtering\\
      -I\$VTK_SOURCE/Common\\
      -I\$VTK_SOURCE/Widgets\\
      -I\$VTK_SOURCE/Rendering\\
      -I\$VTK_SOURCE/Rendering/Testing/Cxx\\
      -I\$VTK_SOURCE/Patented\\
      -I\$VTK_SOURCE/Graphics\\
      -I\$VTK_SOURCE/Utilities"
VISIT_VTK_CPPFLAGS="\$VISIT_VTK_CPPFLAGS -I\$VTK_BUILD/VTK -I\$VTK_BUILD/VTK/Utilities"
VISIT_VTK_LDFLAGS="\\
     -rdynamic\\
     -L\$VTK_BUILD/bin\\
     -L${SUPPORT_DIR}/mpich2-1.0.8/lib\\
     -lvtkFiltering\\
     -lvtkHybrid\\
     -lvtkParallel\\
     -lvtkGraphics\\
     -lvtkImaging\\
     -lvtkRendering\\
     -lvtkGraphics\\
     -lvtkImaging\\
     -lvtkftgl\\
     -lvtkfreetype\\
     -lGL -lXt -lSM -lICE -lX11 -lXext\\
     -lvtkIO\\
     -lvtkDICOMParser\\
     -lvtkmetaio\\
     -lvtksqlite\\
     -lvtkpng\\
     -lvtktiff\\
     -lvtkzlib\\
     -lvtkjpeg\\
     -lvtkexpat\\
     -lvtkexoIIc\\
     -lvtkNetCDF\\
     -lvtkverdict\\
     -lvtkFiltering\\
     -lvtkCommon\\
     -lvtksys\\
     -ldl\\
     -lm\\
     -Wl,-rpath,\$VTK_BUILD/bin:${SUPPORT_DIR}/mpich2-1.0.8/lib\\
     -lmpichcxx -lmpich -lpthread -lrt"

VTK_INCLUDE=\$VISIT_VTK_CPPFLAGS

# MESA=${SUPPORT_DIR}/Mesa-7.2
# MESA_INCLUDE=${SUPPORT_DIR}/Mesa-7.2/include
# MESA_LIBS=-L${SUPPORT_DIR}/Mesa-7.2/lib

# QT, is required by the build system eg. xmlToMakefile
QT_BIN=${SUPPORT_DIR}/qt-3.3.8/bin
QT_INCLUDE=${SUPPORT_DIR}/qt-3.3.8/include
QT_LIB=${SUPPORT_DIR}/qt-3.3.8/lib

# # NOTE Also have to modify configure.in add version test for 4.4.3 
# # other issues: include structure.
# QT_BIN=/usr/bin
# QT_INCLUDE=/usr/include/qt4/Qt
# QT_LIB=/usr/lib

# Compiler flags.
CC="gcc"
CXX="g++"
CFLAGS="-g -Wno-deprecated"
CXXFLAGS="-fPIC -g -Wno-deprecated -DMPICH_IGNORE_CXX_SEEK -I${SUPPORT_DIR}/mpich2-1.0.8/include \$CXXFLAGS"
CPPFLAGS="-fPIC \$VISIT_VTK_CPPFLAGS -g -Wno-deprecated \$CPPFLAGS"
MPI_LIBS="-L${SUPPORT_DIR}/mpich2-1.0.8/lib -Wl,-rpath -Wl,${SUPPORT_DIR}/mpich2-1.0.8/lib -lmpichcxx -lmpich -lpthread -lrt "

# Database reader plugin support libraries
DEFAULT_SZIP_INCLUDE=${SUPPORT_DIR}/szip-2.1/include
DEFAULT_SZIP_LIB=${SUPPORT_DIR}/szip-2.1/lib
DEFAULT_HDF4_INCLUDE=${SUPPORT_DIR}/hdf4-4.2r4/include
DEFAULT_HDF4_LIBS=${SUPPORT_DIR}/hdf4-4.2r4/lib
DEFAULT_HDF5_INCLUDE=${SUPPORT_DIR}/hdf5-1.6.8_ser/include
DEFAULT_HDF5_LIB=${SUPPORT_DIR}/hdf5-1.6.8_ser/lib
DEFAULT_NETCDF_INCLUDE=${SUPPORT_DIR}/netcdf-3.6.0-p1/include
DEFAULT_NETCDF_LIB=${SUPPORT_DIR}/netcdf-3.6.0-p1/lib
DEFAULT_SILO_INCLUDES=${SUPPORT_DIR}/silo-4.6.2/include
DEFAULT_SILO_LIBRARY=${SUPPORT_DIR}/silo-4.6.2/lib
DEFAULT_BOXLIB2D_INCLUDE=${SUPPORT_DIR}/boxlib/include/2D
DEFAULT_BOXLIB2D_LIBS=${SUPPORT_DIR}/boxlib/lib
DEFAULT_BOXLIB3D_INCLUDE=${SUPPORT_DIR}/boxlib/include/3D
DEFAULT_BOXLIB3D_LIBS=${SUPPORT_DIR}/boxlib/lib
DEFAULT_CFITSIO_INCLUDE=${SUPPORT_DIR}/cfitsio/include
DEFAULT_CFITSIO_LIB=${SUPPORT_DIR}/cfitsio/lib
DEFAULT_H5PART_INCLUDE=${SUPPORT_DIR}/h5part-1.3.3/include
DEFAULT_H5PART_LIB=${SUPPORT_DIR}/h5part-1.3.3/lib
DEFAULT_CGNS_INCLUDE=${SUPPORT_DIR}/cgns-2.4/include
DEFAULT_CGNS_LIB=${SUPPORT_DIR}/cgns-2.4/lib
DEFAULT_GDAL_INCLUDE=${SUPPORT_DIR}/gdal-1.6.0/include
DEFAULT_GDAL_LIB=${SUPPORT_DIR}/gdal-1.6.0/lib
# DEFAULT_CCMIO_INCLUDE=
# DEFAULT_CCMIO_LIB=
# BV_MILI_DIR=
# DEFAULT_VISUS_INCLUDE=
# DEFAULT_VISUS_LIB=
# DEFAULT_ITAPS_INCLUDE=
# DEFAULT_ITAPS_LIB=
# DEFAULT_EXODUS_INCLUDES=
# DEFAULT_EXODUS_LIBRARY=

EOF

  export QTDIR=${SUPPORT_DIR}/qt-x11-free-3.3.8
  export LD_LIBRARY_PATH=$QTDIR/lib
  cd ${SUPPORT_DIR}/VisIt-1.10.0.X-all/VisItDev1.10.0.X/src
#  make distclean
  ./configure --prefix=${SUPPORT_DIR}/VisIt-1.10.0 --with-config=${SUPPORT_DIR}/VisIt-1.10.0.X-all/vtkVisitDatabaseBridge.conf --with-hdf5=${SUPPORT_DIR}/hdf5-1.6.8_ser/include,${SUPPORT_DIR}/hdf5-1.6.8_ser/lib --enable-parallel --disable-scripting --disable-visitmodule --disable-viewer-mesa-stub --disable-icet --disable-bilib --disable-glew --disable-bzip2 --with-dbs=all --with-silo-include=${SUPPORT_DIR}/silo-4.6.2/include --with-silo-library=${SUPPORT_DIR}/silo-4.6.2/lib
  make -j${CORES}
fi

cd ${PV_BIN}
echo "Reconfiguring and rebuilding in ${builddir}"

export LD_LIBRARY_PATH=${SUPPORT_DIR}/qt-4.3.5/bin/lib:${SUPPORT_DIR}/ffmpeg/lib:${SUPPORT_DIR}/python25/lib

rm CMakeCache.txt

cat >> CMakeCache.txt << EOF
CMAKE_BUILD_TYPE:STRING=Release
BUILD_SHARED_LIBS:BOOL=ON
VTK_USE_RPATH:BOOL=OFF
PARAVIEW_BUILD_QT_GUI:BOOL=ON
QT_QMAKE_EXECUTABLE:FILEPATH=${SUPPORT_DIR}/qt-4.3.5/bin/bin/qmake
VTK_USE_QVTK_QTOPENGL:BOOL=ON
VTK_USE_64BIT_IDS:BOOL=OFF
PARAVIEW_USE_SYSTEM_HDF5:BOOL=ON
HDF5_LIBRARY:PATH=${SUPPORT_DIR}/hdf5-1.6.8_ser/lib/libhdf5.a;${SUPPORT_DIR}/szip-2.1/lib/libsz.a
HDF5_INCLUDE_DIR:PATH=${SUPPORT_DIR}/hdf5-1.6.8_ser/include
PARAVIEW_BUILD_PLUGIN_VisItReaderPlugin:BOOL=ON
VISIT_BASE:PATH=${SUPPORT_DIR}/VisIt-1.10.0.X-all/VisItDev1.10.0.X
VTK_USE_FFMPEG_ENCODER:BOOL=ON
FFMPEG_INCLUDE_DIR:PATH=${SUPPORT_DIR}/ffmpeg/include/
FFMPEG_avcodec_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavcodec.so
FFMPEG_avformat_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavformat.so
FFMPEG_avutil_LIBRARY:FILEPATH=${SUPPORT_DIR}/ffmpeg/lib/libavutil.so
PARAVIEW_ENABLE_PYTHON:BOOL=ON
PARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
PYTHON_EXECUTABLE:PATH=${SUPPORT_DIR}/python25/bin/python
PYTHON_INCLUDE_PATH:PATH=${SUPPORT_DIR}/python25/include/python2.5/
PYTHON_LIBRARY:PATH=${SUPPORT_DIR}/python25/lib/libpython2.5.so
PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin:BOOL=ON
EOF

cmake ${PV_SRC}
make -j${CORES}

echo "Building... Documentation "
make HTMLDocumentation

echo "Generating package using CPACK"
cpack -G TGZ

package_name=`ls -1 | grep tar | cut -f-3 -d.`
echo "The package is ${package_name}"
tar zxf ${package_name}.tar.gz

lib_dir_name=`ls ${package_name}/lib | grep paraview`
lib_dir=${package_name}/lib/${lib_dir_name}

# Now add some standard libraries that don't get installed using cmake rules to the package.
cd ${lib_dir}
ls
cp   /usr/lib/libstdc++.so.6 /lib/libgcc_s.so.1 ${SUPPORT_DIR}/python25/lib/libpython2.5.so.1.0 /usr/lib/libpng12.so.0 /usr/lib/libfontconfig.so.1 /usr/lib/libfreetype.so.6 /usr/lib/libz.so.1 /usr/lib/libexpat.so.1 ./
rm -f *.debug
cp -r ${SUPPORT_DIR}/python25/lib/ .
cd ${PV_BIN}
tar zcf ${package_name}.tar.gz ${package_name}
cp ${package_name}.tar.gz ../../
echo "Package build successfully: ${builddir}/../${package_name}.tar.gz"


