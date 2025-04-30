#!/bin/sh

set -e

# Install EPEL
dnf install -y --setopt=install_weak_deps=False \
    epel-release

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False --nogpgcheck \
    bzip2 patch doxygen git-core git-lfs

# Qt dependencies
dnf install -y --setopt=install_weak_deps=False --nogpgcheck \
    qt5-qtbase-devel qt5-qttools-devel qt5-qtsvg-devel qt5-qtxmlpatterns-devel

# GNOME theme requirements
dnf install -y --setopt=install_weak_deps=False \
    abattis-cantarell-fonts

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    libasan libtsan libubsan clang-tools-extra \
    gcc gcc-c++ gcc-gfortran \
    ninja-build

# External dependencies
dnf install -y --setopt=install_weak_deps=False --nogpgcheck \
    libXcursor-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel boost-devel gdal-devel PDAL-devel \
    cgnslib-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False --nogpgcheck \
    python3.11 python3.11-devel python3.11-numpy python3.11-pip

python3.11 -m pip install \
    wslink==2.0.3 \
    twisted==24.3.0 \
    autobahn==23.1.2 \
    pandas==2.0.3 \
    sphinx==7.1.2 \
    matplotlib==3.7.5

(
    . /etc/profile.d/modules.sh
    module load mpi/mpich-x86_64
    python3.11 -m pip install \
        'mpi4py[mpich]==3.1.6'
)

# Plugin dependencies
dnf install -y --setopt=install_weak_deps=False \
    libcurl-devel

# Upgrade libarchive (for CMake)
dnf upgrade -y --setopt=install_weak_deps=False \
    libarchive

dnf clean all
