## NVIDIA IndeX plugin improvements

### Multi-GPU and cluster rendering available by default

The NVIDIA IndeX plugin now comes with a free evaluation license that enables all features for a limited time, including
full scalability to run on multiple NVIDIA GPUs and on a cluster of GPU hosts. NVIDIA IndeX will continue to run after
the evaluation period, but with multi-GPU features disabled. Please see the NVIDIA IndeX ParaView Plugin User Guide for
details.

### Optimizations for unstructured grids

Generating the on-device acceleration structure used by NVIDIA IndeX for unstructured grid data is significantly faster
now, thanks to optimizations that make use of the available GPU hardware resources. This can speed up the time to first
rendering with the "NVIDIA IndeX" representation by 2x or more.

### NVIDIA IndeX upgraded to CUDA 12, new driver/GPU requirements

NVIDIA IndeX now uses CUDA 12.3, which has improved support for new GPU architectures.

The minimum NVIDIA driver versions required are 525.60.13 (Linux) and 527.41 (Windows). Recommended driver versions are
545.23.06 (Linux) and 545.84 (Windows) or newer.

CUDA 12 requires an NVIDIA GPU that supports at least CUDA compute capability 5.0, i.e., has "Maxwell" GPU architecture
or newer. Support for the older "Kepler" GPU architecture (e.g., NVIDIA Tesla K10, K40, K80) was removed. Information
about the compute capability of a specific GPU model can be found on the
[NVIDIA website](https://developer.nvidia.com/cuda-gpus).

### Added support for ARM architecture (aarch64) on Linux

Linux binaries of the NVIDIA IndeX library are now also provided for the ARM architecture (aarch64).

Please see the NVIDIA IndeX ParaView Plugin User Guide for details on how to build ParaView with NVIDIA IndeX on this
platform.

### Removed support for POWER9 architecture (ppc64le)

The POWER9 architecture (ppc64le) is no longer supported by the NVIDIA IndeX plugin.

### Additional improvements

- Data that is outside the camera's view frustum will now be imported immediately by the NVIDIA IndeX plugin.
  Previously, data import could get delayed until the camera was moved, which would temporarily interrupt user interaction.
