#!/bin/sh

set -e


# mirrorlist.centos.org no longer exists because el7 is past end of life.
# To get packages, replace mirrorlist with baseurl and change mirror.centos.org
# to vault.centos.org.
sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

# Install extra dependencies for VTK
yum install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core

# Python dependencies
yum install -y --setopt=install_weak_deps=False \
    centos-release-scl

# Clean up additional repos configured by centos-release-scl
sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

# Core build tools
yum install -y --setopt=install_weak_deps=False \
    devtoolset-8

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

# EPEL for more tools
yum install -y --setopt=install_weak_deps=False \
    epel-release

# Development tools not in EL7
yum install -y --setopt=install_weak_deps=False \
    git-lfs

yum clean all
