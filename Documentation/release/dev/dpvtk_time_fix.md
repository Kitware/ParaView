## DPvtk reader fixes

The **DPvtk MultiBlock Data Reader** in the DPvtkReader plugin now reads all the time steps stored in a DPvtk file. Previously, only the first time step was read. For a requested time, the reader loads the first time step with a time greater than or equal to it, or the last time step, the same as the PVD reader.

The reader is also more robust:

* It no longer leaks memory each time it loads a time step.
* Errors while reading a DPvtk file are now reported instead of being ignored. When reading in parallel, an error on one process is reported on all processes instead of making the other processes hang.
* A time step that stores two partition files with the same name is now reported as an error instead of silently reading the wrong data.

A DPvtk file stores all the time steps, so you can no longer open several DPvtk files as a file series.

### Developer notes

The DPvtkReader plugin now requires the `VTK::ParallelCore` module, and it uses the MPI version of DPvz only when the `VTK::ParallelMPI` module is enabled.
