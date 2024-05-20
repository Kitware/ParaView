#!/bin/sh

set -e

# Install extra dependencies for VTK
yum install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core

# Core build tools
yum install -y --setopt=install_weak_deps=False \
    gcc gcc-c++

# MPI dependencies
yum install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# Qt dependencies
yum install -y --setopt=install_weak_deps=False \
    qt5-qtbase-devel

# Mesa dependencies
yum install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers mesa-libGL* glx-utils

# External dependencies
yum install -y --setopt=install_weak_deps=False \
    libXcursor-devel

# Python dependencies
yum install -y --setopt=install_weak_deps=False \
    centos-release-scl

# Python dependencies
yum install -y --setopt=install_weak_deps=False \
    rh-python38-python3 rh-python38-python-devel rh-python38-python3-numpy \
    rh-python38-python3-pip rh-python38-python3-mpi4py-mpich \
    rh-python38-python3-mpi4py-openmpi rh-python38-python3-matplotlib

# EPEL for more tools
yum install -y --setopt=install_weak_deps=False \
    epel-release

# Development tools not in EL7
yum install -y --setopt=install_weak_deps=False \
    git-lfs

yum clean all
