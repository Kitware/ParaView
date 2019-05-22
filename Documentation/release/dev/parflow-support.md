# ParFlow simulation support

* ParaView can now read ParFlow's simulation output
  (files named `.pfb` and `.C.pfb`).
* Preliminary support for reading multiple ParFlow
  files onto a single pair of meshes (surface and
  subsurface) is also provided; it expects JSON
  files with a `.pfmetadata` extension.
+ Example python filters are included to calculate
    + total subsurface storage
    + water table depth at every surface point
    + total water balance (including subsurface
      storage, surface storage, and surface runoff).
