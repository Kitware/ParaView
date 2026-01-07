## Add Atmosphere readers and filters for EAM/E3SM data

ParaView now includes Python-based readers and filters for visualizing E3SM/EAM
(Energy Exascale Earth System Model / E3SM Atmosphere Model) atmospheric data
stored in NetCDF format.

### Readers

- **Atmosphere 3D Data Reader**: Multi-output reader producing separate outputs
  for 2D surface variables, 3D middle layer (lev), and 3D interface layer (ilev)
  variables. Supports time animation and automatic detection of hybrid
  sigma-pressure coordinates.

- **Atmosphere 2D Data Reader**: Single-output reader with vertical level slicing,
  useful for lightweight 2D visualizations of specific atmospheric layers.

### Filters

- **Atmosphere Volumize**: Converts stacked 2D quad layers into 3D hexahedral cells
  for volume rendering.

- **Atmosphere Extract Slices**: Extracts a contiguous range of vertical levels
  from 3D data.

- **Atmosphere Sphere**: Projects lat/lon data onto a 3D sphere for globe
  visualization with optional vertical exaggeration.

### Requirements

The readers and filters are automatically available when the `netCDF4` Python
module is installed.
