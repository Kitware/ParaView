## NVIDIA IndeX plugin improvements

### Unstructured grids

The NVIDIA IndeX plugin is now more efficient when loading and rendering large
unstructured grids, thanks to an optimized data subdivision that does not
require any manual configuration.

The unstructured grid renderer in NVIDIA IndeX has been made more robust when
handling grids with degenerate cells or topology issues.

Support for rendering unstructured grids with per-cell attributes (scalars) was
also added.

### Structured grids

When rendering structured grids on multiple ranks with the NVIDIA IndeX plugin,
ParaView now uses significantly less main memory than before.

### Multi-GPU rendering

The GPUs used by NVIDIA IndeX are now decoupled from those assigned to ParaView
ranks. This means all available GPUs will be utilized for rendering, independent
of the number of ranks. Running ParaView on multiple ranks is still beneficial,
however, for accelerating data loading and processing, as well as for other
rendering modes.

Please note that multi-GPU support requires the Cluster Edition of the NVIDIA
IndeX plugin.

### POWER9 support

The NVIDIA IndeX library is now also provided for the POWER9 architecture
(ppc64le), enabling users to build and run ParaView with the NVIDIA IndeX plugin
on that platform. On POWER9, the plugin automatically comes with all features of
the Cluster Edition, supporting multi-GPU and multi-node use cases out of the
box.

### Stability fixes and usability improvements

The plugin will no longer try to initialize CUDA on a ParaView client that is
connected to a remote `pvserver`. This previously caused errors when CUDA was
not available on the client.

If ParaView is running on multiple ranks, the plugin requires IceT compositing
to be disabled by the user. It will now inform the user if IceT has yet not been
disabled in that case.

When running on multiple ranks and loading a dataset using a file reader that
does not support parallel I/O (such as the legacy VTK reader), ParaView will
load all data on a single rank. If this happens, the plugin will now inform the
user that only a single rank will be used for rendering with NVIDIA IndeX and
recommend switching to a file format that has a parallel reader.

Enabling the "NVIDIA IndeX" representation for multiple visible datasets is now
handled more consistently: The plugin will only render the visible dataset that
was added last (i.e. the last entry shown in the Pipeline Browser) and print a
warning informing the user about this. Several issues when switching between
datasets by toggling their visibility were resolved.

Logging of errors and warnings for error conditions and unsupported use cases
was improved.
