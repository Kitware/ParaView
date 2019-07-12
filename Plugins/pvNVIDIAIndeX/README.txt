NVIDIA IndeX for ParaView Plugin

The NVIDIA IndeX for ParaView Plugin enables the large-scale volume data
visualization capabilities of the NVIDIA IndeX library inside Kitware's ParaView.
This document will provide a brief overview of the installation package, please
refer to the user guide for detailed instructions.

#-------------------------------------------------------------------------------
# Package Structure
#-------------------------------------------------------------------------------

 installation-directory
    |
    | -- doc                User's Guide for the NVIDIA IndeX for ParaView Plugin.
    |                       The users guide provides detailed installation
    |                       instructions, describes the implemented feature set
    |                       supported by the plugin, answers frequently asked
    |                       questions and lists limitations and knows issues.
    |
    | -- lib                NVIDIA IndeX and DiCE libraries specific
    |                       to your platform.
    |
    | -- src                 ParaView plugin source code
    |
    | -- EULA.pdf           NVIDIA End User License Agreement.
    |
    | -- license.txt        Third-party licensing information.
    |
    | -- README.txt         This file.
    |

#-------------------------------------------------------------------------------
# Compatibility and Prerequisites
#-------------------------------------------------------------------------------

The NVIDIA IndeX for ParaView Plugin is compatible with:

* ParaView-5.5.0 and later.
  (depending on the plugin version and downloaded package, Windows 64-bit).
* OpenMPI-1.7.4 and later.
  (if running in client-server mode).
* NVIDIA IndeX 2.0 / NVIDIA IndeX 2.1 (starting with ParaView 5.7)
  (installed with the ParaView plugin).
* NVIDIA GPU(s) supporting CUDA compute capability 3.0 or higher, i.e. Kepler
  GPU architecture generation or later.
* NVIDIA display driver version 387.26 or later on Linux and
  391.03 or later on Windows.

#-------------------------------------------------------------------------------
# Features and Licensing
#-------------------------------------------------------------------------------

The NVIDIA IndeX for ParaView Plugin comes with a free workstation license that
enables exploiting the capabilities of a single GPU.

If you aim to use NVIDIA IndeX on a cluster of multiple hosts and/or
with multiple NVIDIA GPUs, then please contact us for the appropriate licensing.
paraview-plugin-support@nvidia.com

# Features
----------

* Real-time and interactive high-quality volume data visualization of both,
  structured and unstructured volume grids

* Interactive visualization of time varying structured volume grids.

* Supporting 8-bit and 16-bit unsigned int, and 32-bit floating point volume
  data types (64-bit floating point is supported via conversion).

* Advanced volume rendering techniques with three different configurable presets 
  for Iso-surfaces, Depth enhancement and Edge enhancement.

* Multiple, axis-aligned volume slice rendering combined with volumetric data.

* Catalyst support for regular grids to do in-situ based visualization.

* User defined region of interest selection.

* Advanced filtering and pre-integration techniques enabling high-fidelity
  visualizations.

* Depth-correct integration of ParaView geometry rendering into NVIDIA IndeX
  volume rendering.
  
* Multiple volume support in scene graph. 
  (limited to one visible volume at time for the current version)

* Free single GPU version for leveraging today's GPU performance.

* Multi-GPU and GPU cluster support for scalable real-time visualization of
  datasets of arbitrary sizes. Requires an appropriate license
  (please contact us).

#-------------------------------------------------------------------------------
# Known Limitations and Bugs in the BETA release
#-------------------------------------------------------------------------------

# Regular volume grids
-------------------------

* Datasets larger than a single GPU's available memory cannot be rendered.
  This issue is plugin specific and shall be resolved in the next release
  version.

* Datasets in *.vtk format won’t distribute to the PV Server and cause
  errors. Please use *.pvti or any other format instead for distributed data
  This issue is ParaView specific. Please contact Kitware for additional details.

# Unstructured volume grids
---------------------------

* Datasets containing degenerated faces may result in incorrect renderings
  or cause ParaView to fail. The NVIDIA IndeX for ParaView Plugin tries to resolve
  all invalid faces automatically.

# Auxiliary
-----------

* The Windows version of the NVIDIA IndeX plugin for ParaView is restricted
  to run on a single workstation/computer only, i.e., cluster rendering
  is not supported on Windows platforms.

* When loading an older state file with both volumetric and geometry data without
  NVIDIA IndeX representation saved in it, first frame shows only volumetric data and when you
  interact the subsequent frames are correct again with both geometry and volume data.

* When using further NVIDIA ARC software installations (such as the NVIDIA IndeX
  native library) in parallel, please avoid conflicts between library versions,
  e.g., by setting your environment LD_LIBRARY_PATH appropriately.

#-------------------------------------------------------------------------------
# Contact
#-------------------------------------------------------------------------------

Please do not hesitate to contact us on the NVIDIA IndeX for ParaView Plugin
forum for further assistance: https://devtalk.nvidia.com/default/board/323/index-paraview/

Support mailing list: paraview-plugin-support@nvidia.com

Copyright 2019 NVIDIA Corporation. All rights reserved.
