## Add EAM reader and filter for E3SM data

ParaView now includes a Python-based reader and filter for visualizing E3SM/EAM
(Energy Exascale Earth System Model / E3SM Atmosphere Model) atmospheric data
stored in NetCDF format.

### Reader

- **EAM Data Reader**: Multi-output reader producing separate outputs for:
  - 2D surface variables as quad mesh
  - 3D middle layer (lev) variables as hexahedral volume mesh
  - 3D interface layer (ilev) variables as hexahedral volume mesh

  The 3D outputs are volumized directly in the reader, producing hexahedral cells
  that span between adjacent vertical levels. This enables immediate volume
  rendering without requiring additional filters. Supports time animation and
  automatic detection of hybrid sigma-pressure coordinates.

### Filter

- **EAM Project To Sphere**: Projects lat/lon data onto a 3D sphere for globe
  visualization with optional vertical exaggeration.

### Requirements

The reader and filter are automatically available when the `netCDF4` Python
module is installed.
