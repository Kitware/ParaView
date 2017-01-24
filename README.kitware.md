# CGNS fork for ParaView

This branch contains changes required to embed CGNS into ParaView. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add attributes to pass commit checks within ParaView.
  * Blocking out flags which ParaView does not care about in CMake primarily:
    - disabling Fortran support;
    - enforcing HDF5 support;
    - simplifying various checks by using ParaView's results rather than
      detecting them again;
    - following ParaView's build options rather than exposing more options to
      users; and
    - building and installing via ParaView's mechanisms.
  * Mangle all exported symbols to have a `vtkcgns_` prefix.
