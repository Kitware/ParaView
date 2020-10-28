NVIDIA IndeX for ParaView Plugin

The NVIDIA IndeX for ParaView Plugin enables the large-scale volume data
visualization capabilities of the NVIDIA IndeX library inside Kitware's ParaView.
This document will provide a brief overview of the installation package, please
refer to the User's Guide for detailed instructions.

#-------------------------------------------------------------------------------
# Compatibility and Prerequisites
#-------------------------------------------------------------------------------

The NVIDIA IndeX for ParaView Plugin is compatible with:

* All NVIDIA GPUs that support at least CUDA compute capability 3.0, i.e.
  "Kepler" GPU architecture or newer. To find out the compute capability of a
  specific GPU, go to: https://developer.nvidia.com/cuda-gpus

* NVIDIA display driver that supports CUDA 10.2.
  Minimum driver version:
  - Linux:   440.33
  - Windows: 441.22

* Operating systems:
  - Red Hat Enterprise Linux (RHEL) or CentOS version 7 or newer.
    The NVIDIA IndeX for ParaView Plugin will typically also run on other Linux
    distributions.
  - Microsoft Windows 10.

#-------------------------------------------------------------------------------
# Features and Licensing
#-------------------------------------------------------------------------------

The NVIDIA IndeX for ParaView Plugin comes with a free workstation license that
enables utilizing the capabilities of a single NVIDIA GPU.

If you want to use NVIDIA IndeX on a cluster of multiple hosts and/or
with multiple NVIDIA GPUs, then please contact NVIDIA for the appropriate licensing:
nvidia-index@nvidia.com.

# Features
----------

* Real-time and interactive high-quality volume data visualization of both
  structured and unstructured volume grids

* Interactive visualization of time-varying structured volume grids.

* Support for 8-bit and 16-bit signed/unsigned integer, and 32-bit floating
  point volume data types. 64-bit floating point is supported via automatic
  conversion.

* XAC (NVIDIA IndeX Accelerated Computing) visual elements for volumes
  with four different configurable presets: Iso-surfaces, depth enhancement,
  edge enhancement, gradient.

* User-defined programmable XAC visual element for volumes.

* Multiple, axis-aligned volume slice rendering combined with volumetric data.

* Catalyst support for regular grids to perform in-situ based visualization.

* User-defined region of interest selection.

* Advanced filtering and pre-integration techniques enabling high-fidelity
  visualizations.

* Depth-correct integration of ParaView geometry rendering into NVIDIA IndeX
  volume rendering.

* Multi-volume support in the scene graph (currently limited to one visible
  volume at a time).

* Multi-GPU and GPU cluster support for scalable real-time visualization of
  datasets of arbitrary size. Requires an appropriate license
  (please contact NVIDIA).

#-------------------------------------------------------------------------------
# Known Limitations
#-------------------------------------------------------------------------------

# Regular volume grids
-------------------------

* When running with MPI on multiple pvserver ranks, datasets in *.vtk format
  won't get distributed to multiple ranks by ParaView, and only a single GPU
  will be utilized. The NVIDIA IndeX for ParaView Plugin will print a warning if
  this case is detected. Please use a different data format such as *.pvti that
  supports distributed data. Since this issue is ParaView specific, please
  contact Kitware for additional details.

# Unstructured grids
--------------------

* Datasets containing degenerate faces may result in incorrect renderings or
  cause ParaView to fail. The NVIDIA IndeX for ParaView Plugin will try to
  resolve all invalid faces automatically.

# General
----------

* The Windows version of the NVIDIA IndeX plugin for ParaView is restricted
  to run on a single workstation/computer only, i.e., cluster rendering
  is not supported on Windows platforms.

* When loading an older state file with both volumetric and geometry data
  without the NVIDIA IndeX representation saved in it, the first frame will show
  only volumetric data. After interacting with the scene, the subsequent frames
  will be correct again with both geometry and volume data.

#-------------------------------------------------------------------------------
# Contact
#-------------------------------------------------------------------------------

Please do not hesitate to contact NVIDIA for further assistance:

Support mailing list: paraview-plugin-support@nvidia.com

NVIDIA IndeX/ParaView forum: https://forums.developer.nvidia.com/c/164

NVIDIA IndeX for ParaView website: https://www.nvidia.com/en-us/data-center/index-paraview-plugin

NVIDIA IndeX website: https://developer.nvidia.com/nvidia-index


Copyright 2020 NVIDIA Corporation. All rights reserved.
