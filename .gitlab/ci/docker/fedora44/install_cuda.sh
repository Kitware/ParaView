#!/bin/sh

set -e

# Install tools to manage repositories.
dnf install -y --setopt=install_weak_deps=False \
    'dnf-command(config-manager)'

# Install the CUDA repository.
dnf config-manager addrepo \
    --from-repofile=https://developer.download.nvidia.com/compute/cuda/repos/fedora44/x86_64/cuda-fedora44.repo

# CUDA toolchain
dnf install -y --setopt=install_weak_deps=False \
    cuda-compiler-13-3 cuda-cudart-devel-13-3 cuda-toolkit-13-3

dnf clean all
