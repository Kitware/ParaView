## NetCDF UGRID Reader

ParaView now provides a new reader to load NetCDF files that follow the [UGRID conventions](https://ugrid-conventions.github.io/ugrid-conventions/).

Only 2D meshes are supported and you can extract points, cells and data arrays associated to them.
You can also replace values denoted as "fill values" with NaN.
