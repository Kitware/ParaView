## vtkPVVersion

* Most of the version macros have been relocated to `vtkPVVersionQuick.h`. The
  new `PARAVIEW_VERSION_EPOCH` macro is defined as the actual
  `PARAVIEW_VERSION_PATCH` value for release-track development and a "high"
  value for future ParaView release development (i.e., the next minor version
  bump). The actual patch number, and `PARAVIEW_VERSION_FULL` values are
  available in `vtkPVVersion.h`. A new `PARAVIEW_VERSION_NUMBER_QUICK` macro
  provides the ParaView version as a comparable value with
  `PARAVIEW_VERSION_CHECK` using the `PARAVIEW_VERSION_EPOCH` instead of
  `PARAVIEW_VERSION_PATCH`. The intent is to reduce the rebuild frequency
  needed for the nightly build version bump for the in-development source.
