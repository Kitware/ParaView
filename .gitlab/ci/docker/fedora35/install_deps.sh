#!/bin/sh

set -e

dnf install -y --setopt=install_weak_deps=False \
  glx-utils mesa-dri-drivers mesa-libGL* libxcrypt-compat.x86_64 libxkbcommon \
  libxkbcommon-x11

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch doxygen git-core git-lfs

# MPI dependencies
dnf install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# Qt dependencies
dnf install -y --setopt=install_weak_deps=False \
    qt5-qtbase-devel qt5-qtbase-private-devel qt5-qttools-devel qt5-qtsvg-devel \
    qt5-qtxmlpatterns-devel qt5-qtmultimedia-devel

# GNOME theme requirements
dnf install -y --setopt=install_weak_deps=False \
    abattis-cantarell-fonts

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    libasan libtsan libubsan clang-tools-extra \
    gcc gcc-c++ gcc-gfortran \
    ninja-build

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel libharu-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel double-conversion-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel boost-devel gdal-devel PDAL-devel \
    cgnslib-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3-twisted python3-autobahn python3 python3-devel python3-numpy \
    python3-pandas python3-pandas-datareader python3-sphinx python3-pip \
    python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

python3 -m pip install wslink cftime

# Plugin dependencies
dnf install -y --setopt=install_weak_deps=False \
    gmsh-devel libcurl-devel openxr openxr-devel

# External repository support
dnf install -y --setopt=install_weak_deps=False \
    dnf-plugins-core

# Openturns dependencies
# Disabling for now because Fedora 35 is no longer provided by the OpenSuse science team.
# dnf config-manager --add-repo https://download.opensuse.org/repositories/science:/openturns/Fedora_35/science:openturns.repo
# dnf install -y --setopt=install_weak_deps=False \
#     openturns-libs openturns-devel

# RPMFusion
dnf install -y --setopt=install_weak_deps=False \
    https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-35.noarch.rpm

dnf install -y --setopt=install_weak_deps=False \
    ffmpeg-devel

dnf clean all
