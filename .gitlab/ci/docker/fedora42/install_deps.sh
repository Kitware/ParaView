#!/bin/sh

set -e

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch pigz git-core git-lfs rsync

# Documentation tools
dnf install -y --setopt=install_weak_deps=False \
    doxygen perl-Digest-MD5

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    libasan libtsan libubsan clang-tools-extra \
    gcc gcc-c++ gcc-gfortran

# MPI dependencies
dnf install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# External repository support
dnf install -y --setopt=install_weak_deps=False \
    dnf-plugins-core

# RPMFusion
dnf install -y --setopt=install_weak_deps=False \
    https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-42.noarch.rpm

# RPMFusion external dependencies
dnf install -y --setopt=install_weak_deps=False \
    ffmpeg-devel

# Qt6 dependencies
dnf install -y --setopt=install_weak_deps=False \
    libxslt xcb-util-cursor

# GNOME theme and language requirements
dnf install -y --setopt=install_weak_deps=False \
    abattis-cantarell-fonts glibc-langpack-en

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-compat-libOSMesa-devel mesa-compat-libOSMesa mesa-dri-drivers mesa-libGL* glx-utils

# Testing dependencies
dnf install -y --setopt=install_weak_deps=False \
    openssh-server

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel libharu-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel boost-devel gdal-devel PDAL-devel \
    cgnslib-devel libxcrypt-compat.x86_64 libxkbcommon libxkbcommon-x11 \
    cmake ninja-build

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3-twisted python3-autobahn python3 python3-devel python3-numpy \
    python3-pandas python3-pandas-datareader python3-sphinx python3-pip \
    python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

python3 -m pip install wslink cftime openPMD-api

# Plugin dependencies
dnf install -y --setopt=install_weak_deps=False \
    gmsh-devel libcurl-devel openxr openxr-devel

# Openturns dependencies
dnf config-manager addrepo \
    --from-repofile=https://download.opensuse.org/repositories/science:/openturns/Fedora_42/science:openturns.repo
dnf install -y --setopt=install_weak_deps=False \
    openturns-libs openturns-devel

dnf clean all
