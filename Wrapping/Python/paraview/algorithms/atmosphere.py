"""
Atmosphere Data - Readers and Filters for ParaView
===================================================

This module provides NetCDF readers and filters for E3SM/EAM (Energy Exascale
Earth System Model / E3SM Atmosphere Model) data visualization in ParaView.

Dependencies
------------
- netCDF4: For reading NetCDF climate data files
- numpy: For numerical array operations
- math: Standard library for spherical coordinate transformations

No external dependencies like pyproj or pandas are required.

Readers
-------
AtmosphereReader3D
    Multi-output reader that produces three separate outputs:
    - Port 0: 2D surface variables (e.g., surface temperature, precipitation)
    - Port 1: 3D variables on middle levels (lev) - cell-centered vertical levels
    - Port 2: 3D variables on interface levels (ilev) - cell-interface vertical levels

AtmosphereReader2D
    Single-output reader that projects 3D data onto a 2D mesh by selecting
    specific vertical level indices. Useful for creating 2D visualizations
    of atmospheric layers without the overhead of full 3D data.

Filters
-------
AtmosphereVolumize
    Converts stacked 2D quad layers from AtmosphereReader3D 3D output into true 3D
    hexahedral cells. Cell data is averaged between adjacent layers.

AtmosphereExtractSlices
    Extracts a contiguous range of vertical slices from 3D atmosphere data.
    Useful for focusing on specific atmospheric regions (e.g., troposphere).

AtmosphereSphere
    Projects lat/lon data onto a 3D sphere for globe visualization.
    Supports scaling for vertical exaggeration of atmospheric layers.

Data Format
-----------
EAM/E3SM data consists of:
- Data files (.nc): NetCDF files containing time-varying atmospheric variables
- Connectivity files (.nc): NetCDF files defining the unstructured mesh topology
  with corner lat/lon coordinates for each grid cell

The readers automatically detect:
- Horizontal dimensions by matching sizes between connectivity and data files
- Vertical dimensions (lev for mid-levels, ilev for interface levels)
- Time dimensions for animation support
- Fill values (_FillValue or missing_value attributes)

Vertical Coordinates
--------------------
EAM uses hybrid sigma-pressure coordinates. The readers can compute pressure
levels from hybrid coefficients (hyam, hybm, hyai, hybi) if explicit level
coordinates are not provided.

Usage Example
-------------
In ParaView:
1. Use File -> Open to select a NetCDF data file
2. Choose "Atmosphere 3D Data Reader" or "Atmosphere 2D Data Reader"
3. Set the connectivity file path in the Properties panel
4. Select variables to load from the array selection widgets
5. Apply to visualize

For spherical projection:
1. Apply AtmosphereReader3D or AtmosphereReader2D reader
2. Apply AtmosphereSphere filter
3. Adjust scale factor for vertical exaggeration if needed
"""

import math

from vtkmodules.numpy_interface import dataset_adapter as dsa
from vtkmodules.vtkCommonCore import vtkPoints, vtkDataArraySelection
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid, vtkCellArray
from vtkmodules.vtkFiltersCore import vtkAppendFilter
from vtkmodules.util import vtkConstants, numpy_support
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
from paraview import print_error, print_warning

try:
    from vtkmodules.vtkFiltersGeneral import vtkCleanUnstructuredGrid as uGridFilter
except ImportError:
    from paraview.modules.vtkPVVTKExtensionsFiltersGeneral import vtkCleanUnstructuredGrid as uGridFilter

try:
    import netCDF4
    import numpy as np
    _has_deps = True
except ImportError as ie:
    print_error(
        "Missing required Python modules/packages. Algorithms in this module may "
        "not work as expected! \n {0}".format(ie))
    _has_deps = False


# ==============================================================================
# Constants and Helper Classes
# ==============================================================================

class AtmosphereConstants:
    """Constants for EAM atmospheric model hybrid coordinates."""
    LEV = 'lev'
    HYAM = 'hyam'
    HYBM = 'hybm'
    ILEV = 'ilev'
    HYAI = 'hyai'
    HYBI = 'hybi'
    P0 = float(1e5)
    PS0 = float(1e5)


class DimMeta:
    """Stores dimension metadata from NetCDF files."""

    def __init__(self, name, size, data=None):
        self.name = name
        self.size = size
        self.long_name = None
        self.units = None
        self.data = data

    def update_from_variable(self, var_info):
        """Update metadata from netCDF variable info."""
        try:
            self.long_name = var_info.getncattr('long_name')
        except AttributeError:
            pass
        try:
            self.units = var_info.getncattr('units')
        except AttributeError:
            pass

    def __repr__(self):
        return f"DimMeta(name='{self.name}', size={self.size})"


class VarMeta:
    """Stores variable metadata from NetCDF files."""

    def __init__(self, name, info, horizontal_dim=None):
        self.name = name
        self.dimensions = info.dimensions
        self.fillval = np.nan
        self.long_name = None
        self.horizontal_dim = horizontal_dim

        # Determine if transpose needed (horizontal dim not last)
        if horizontal_dim and horizontal_dim in self.dimensions:
            self.transpose = self.dimensions.index(horizontal_dim) != len(self.dimensions) - 1
        else:
            self.transpose = False

        # Extract fill value - try both _FillValue and missing_value
        for attr in ['_FillValue', 'missing_value']:
            try:
                self.fillval = info.getncattr(attr)
                break
            except (AttributeError, KeyError):
                pass

        # Get long_name if available
        try:
            self.long_name = info.getncattr('long_name')
        except (AttributeError, KeyError):
            pass

    def has_dim(self, dim_name):
        """Check if variable has a specific dimension."""
        return dim_name in self.dimensions

    def get_vertical_dim(self):
        """Return the vertical dimension (lev or ilev) if present."""
        if AtmosphereConstants.LEV in self.dimensions:
            return AtmosphereConstants.LEV
        elif AtmosphereConstants.ILEV in self.dimensions:
            return AtmosphereConstants.ILEV
        return None

    def __repr__(self):
        return f"VarMeta(name='{self.name}', dims={self.dimensions})"


# ==============================================================================
# Helper Functions
# ==============================================================================

def compare(data, arrays, dim):
    """Compare arrays for hybrid coordinate validation."""
    ref = data[arrays[0]][:].flatten()
    if len(ref) != dim:
        raise Exception("Length of hya_/hyb_ variable does not match the corresponding dimension")
    for array in arrays[1:]:
        comp = data[array][:].flatten()
        if not np.array_equal(ref, comp):
            return None
    return ref


def FindSpecialVariable(data, lev, hya, hyb):
    """Find or compute level coordinates from hybrid coefficients."""
    dim = data.dimensions.get(lev, None)
    if dim is None:
        raise Exception(f'{lev} not found in dimensions')
    dim = dim.size
    var = np.array(list(data.variables.keys()))

    if lev in var:
        return data[lev][:].flatten()

    _hyai = var[np.char.find(var, hya) != -1]
    _hybi = var[np.char.find(var, hyb) != -1]
    if len(_hyai) != len(_hybi):
        raise Exception('Unmatched pair of hya and hyb variables found')

    p0 = AtmosphereConstants.P0
    ps0 = AtmosphereConstants.PS0

    if len(_hyai) == 1:
        hyai = data[_hyai[0]][:].flatten()
        hybi = data[_hybi[0]][:].flatten()
        if not (len(hyai) == dim and len(hybi) == dim):
            raise Exception('Lengths of arrays for hya_ and hyb_ variables do not match')
        return ((hyai * p0) + (hybi * ps0)) / 100.0
    else:
        hyai = compare(data, _hyai, dim)
        hybi = compare(data, _hybi, dim)
        if hyai is None or hybi is None:
            raise Exception('Values within hya_ and hyb_ arrays do not match')
        return ((hyai * p0) + (hybi * ps0)) / 100.0


def createModifiedCallback(anobject):
    """Create a weak-reference callback for ParaView modified events."""
    import weakref
    weakref_obj = weakref.ref(anobject)
    anobject = None

    def _markmodified(*args, **kwargs):
        o = weakref_obj()
        if o is not None:
            o.Modified()
    return _markmodified


def identify_horizontal_dim(meshdata, vardata):
    """
    Identify horizontal dimension from connectivity file and match in data file.
    Returns (conn_dim_name, data_dim_name, size) or (None, None, None) on failure.
    """
    conn_dims = list(meshdata.dimensions.keys())
    if not conn_dims:
        return None, None, None

    # First dimension in connectivity file is typically the horizontal dim
    conn_dim = conn_dims[0]
    conn_size = meshdata.dimensions[conn_dim].size

    # Find matching dimension in data file by size
    for dim_name, dim_obj in vardata.dimensions.items():
        if dim_obj.size == conn_size:
            return conn_dim, dim_name, conn_size

    return conn_dim, None, conn_size


def find_lat_lon_vars(meshdata):
    """Find lat/lon variable names in connectivity file."""
    mvars = np.array(list(meshdata.variables.keys()))
    lat_matches = mvars[np.char.find(mvars, 'corner_lat') > -1]
    lon_matches = mvars[np.char.find(mvars, 'corner_lon') > -1]
    if len(lat_matches) == 0 or len(lon_matches) == 0:
        return None, None
    return lat_matches[0], lon_matches[0]


def build_2d_geometry(lat, lon, ncells):
    """Build VTK geometry arrays for 2D mesh."""
    coords = np.empty((len(lat), 3), dtype=np.float64)
    coords[:, 0] = lon
    coords[:, 1] = lat
    coords[:, 2] = 0.0

    vtk_coords = vtkPoints()
    vtk_coords.SetData(dsa.numpyTovtkDataArray(coords))

    cell_types = np.empty(ncells, dtype=np.uint8)
    cell_types.fill(vtkConstants.VTK_QUAD)
    cell_types = numpy_support.numpy_to_vtk(
        num_array=cell_types, deep=True, array_type=vtkConstants.VTK_UNSIGNED_CHAR)

    offsets = np.arange(0, (4 * ncells) + 1, 4, dtype=np.int64)
    offsets = numpy_support.numpy_to_vtk(
        num_array=offsets, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

    cells = np.arange(ncells * 4, dtype=np.int64)
    cells = numpy_support.numpy_to_vtk(
        num_array=cells, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

    cell_array = vtkCellArray()
    cell_array.SetData(offsets, cells)

    return vtk_coords, cell_types, cell_array


def build_3d_geometry(lat, lon, levels, ncells2d):
    """Build VTK geometry arrays for 3D mesh with vertical levels."""
    nlev = len(levels)
    npts = len(lat)

    # Build 3D coordinates
    coords = np.empty((nlev, npts, 3), dtype=np.float64)
    for i, z in enumerate(levels):
        coords[i, :, 0] = lon
        coords[i, :, 1] = lat
        coords[i, :, 2] = z
    coords = coords.reshape(nlev * npts, 3)

    vtk_coords = vtkPoints()
    vtk_coords.SetData(dsa.numpyTovtkDataArray(coords))

    ncells3d = ncells2d * nlev
    cell_types = np.empty(ncells3d, dtype=np.uint8)
    cell_types.fill(vtkConstants.VTK_QUAD)
    cell_types = numpy_support.numpy_to_vtk(
        num_array=cell_types, deep=True, array_type=vtkConstants.VTK_UNSIGNED_CHAR)

    offsets = np.arange(0, (4 * ncells3d) + 1, 4, dtype=np.int64)
    offsets = numpy_support.numpy_to_vtk(
        num_array=offsets, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

    cells = np.arange(ncells3d * 4, dtype=np.int64)
    cells = numpy_support.numpy_to_vtk(
        num_array=cells, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

    cell_array = vtkCellArray()
    cell_array.SetData(offsets, cells)

    return vtk_coords, cell_types, cell_array


def load_variable_data(vardata, varmeta, time_idx):
    """Load variable data for a given time index, handling transpose and fill values."""
    try:
        if varmeta.transpose:
            data = vardata[varmeta.name][:].data[time_idx].transpose().flatten()
        else:
            data = vardata[varmeta.name][:].data[time_idx].flatten()
        return np.where(data == varmeta.fillval, np.nan, data)
    except Exception as e:
        print_error(f"Error loading variable {varmeta.name}: {e}")
        return np.array([])


def process_point_to_sphere(point, max_z, radius, scale):
    """Convert lat/lon/z point to spherical coordinates."""
    theta = point[0]
    phi = 90 - point[1]
    rho = (max_z - point[2]) * scale + radius if point[2] != 0 else radius
    x = rho * math.sin(math.radians(phi)) * math.cos(math.radians(theta))
    y = rho * math.sin(math.radians(phi)) * math.sin(math.radians(theta))
    z = rho * math.cos(math.radians(phi))
    return [x, y, z]


# ==============================================================================
# AtmosphereReader3D - Multi-output reader for 2D, 3D-lev, 3D-ilev data
# ==============================================================================

class AtmosphereReader3D(VTKPythonAlgorithmBase):
    """
    Multi-output NetCDF reader for E3SM/EAM atmospheric model data.

    This reader produces three separate outputs for different variable types:
    - Port 0 (2D): Surface variables with 2 dimensions (time, ncol)
    - Port 1 (3D-lev): Variables on middle levels with 3 dimensions (time, lev, ncol)
    - Port 2 (3D-ilev): Variables on interface levels with 3 dimensions (time, ilev, ncol)

    The reader automatically detects horizontal dimensions by matching the connectivity
    file's cell count with dimensions in the data file. It handles fill values
    (_FillValue, missing_value) by converting them to NaN.

    Parameters
    ----------
    FileName1 : str
        Path to the NetCDF data file containing atmospheric variables.
    FileName2 : str
        Path to the NetCDF connectivity file containing mesh topology
        (corner_lat, corner_lon arrays defining cell vertices).

    Outputs
    -------
    Port 0 : vtkUnstructuredGrid
        2D surface mesh with quad cells. Each cell corresponds to one
        horizontal grid column. Contains selected 2D variables as cell data.
    Port 1 : vtkUnstructuredGrid
        3D mesh for middle-level (lev) variables. Organized as stacked 2D
        quad layers at each vertical level. Z-coordinate represents pressure
        level. Contains 'lev' and 'numlev' in field data.
    Port 2 : vtkUnstructuredGrid
        3D mesh for interface-level (ilev) variables. Similar structure to
        Port 1 but for variables defined at layer interfaces. Contains 'ilev'
        and 'numilev' in field data.

    Notes
    -----
    - Vertical coordinates are computed from hybrid sigma-pressure coefficients
      (hyam, hybm for lev; hyai, hybi for ilev) if explicit level arrays are missing.
    - The reader caches NetCDF file handles to avoid repeated file opens.
    - Variables are grouped by type in the ParaView UI for easy selection.
    - Time dimension is automatically detected and exposed for animation.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = AtmosphereReader3D()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.UpdatePipelineInformation()
        reader.Get2DDataArrays().EnableArray('TREFHT')
        reader.Update()
    """

    def __init__(self):
        VTKPythonAlgorithmBase.__init__(
            self, nInputPorts=0, nOutputPorts=3, outputType='vtkUnstructuredGrid')

        self._data_filename = None
        self._conn_filename = None
        self._time_steps = []

        self._horizontal_dim = None
        self._data_horizontal_dim = None
        self._dimensions = {}
        self._variables = {}

        self._vars2D_selection = vtkDataArraySelection()
        self._vars3Dm_selection = vtkDataArraySelection()
        self._vars3Di_selection = vtkDataArraySelection()

        for sel in [self._vars2D_selection, self._vars3Dm_selection, self._vars3Di_selection]:
            sel.AddObserver("ModifiedEvent", createModifiedCallback(self))

        self._mesh_dataset = None
        self._var_dataset = None
        self._cached_mesh_filename = None
        self._cached_var_filename = None
        self._area_var = None

    def __del__(self):
        self._close_datasets()

    def _close_datasets(self):
        for ds in [self._mesh_dataset, self._var_dataset]:
            if ds is not None:
                try:
                    ds.close()
                except Exception:
                    pass
        self._mesh_dataset = None
        self._var_dataset = None

    def _get_mesh_dataset(self):
        if self._conn_filename != self._cached_mesh_filename or self._mesh_dataset is None:
            if self._mesh_dataset is not None:
                try:
                    self._mesh_dataset.close()
                except Exception:
                    pass
            self._mesh_dataset = netCDF4.Dataset(self._conn_filename, "r")
            self._cached_mesh_filename = self._conn_filename
        return self._mesh_dataset

    def _get_var_dataset(self):
        if self._data_filename != self._cached_var_filename or self._var_dataset is None:
            if self._var_dataset is not None:
                try:
                    self._var_dataset.close()
                except Exception:
                    pass
            self._var_dataset = netCDF4.Dataset(self._data_filename, "r")
            self._cached_var_filename = self._data_filename
        return self._var_dataset

    def _clear(self):
        self._variables.clear()
        self._dimensions.clear()
        self._time_steps.clear()
        self._horizontal_dim = None
        self._data_horizontal_dim = None
        self._area_var = None

    def _populate_metadata(self):
        if self._data_filename is None or self._conn_filename is None:
            return

        meshdata = self._get_mesh_dataset()
        vardata = self._get_var_dataset()

        self._horizontal_dim, self._data_horizontal_dim, _ = identify_horizontal_dim(meshdata, vardata)
        if not self._data_horizontal_dim:
            print_warning("Could not match horizontal dimension in data file")
            return

        self._vars2D_selection.RemoveAllArrays()
        self._vars3Dm_selection.RemoveAllArrays()
        self._vars3Di_selection.RemoveAllArrays()

        for dim_name, dim_obj in vardata.dimensions.items():
            dim_meta = DimMeta(dim_name, dim_obj.size)
            if dim_name in vardata.variables:
                dim_meta.update_from_variable(vardata.variables[dim_name])
                try:
                    dim_meta.data = vardata[dim_name][:].data
                except Exception:
                    pass
            self._dimensions[dim_name] = dim_meta

        for name, info in vardata.variables.items():
            if self._data_horizontal_dim not in info.dimensions:
                continue

            varmeta = VarMeta(name, info, self._data_horizontal_dim)
            self._variables[name] = varmeta

            if len(info.dimensions) == 1 and 'area' in name.lower():
                self._area_var = varmeta
                continue

            ndims = len(info.dimensions)
            if ndims == 2:
                self._vars2D_selection.AddArray(name)
            elif ndims == 3:
                if AtmosphereConstants.LEV in info.dimensions:
                    self._vars3Dm_selection.AddArray(name)
                elif AtmosphereConstants.ILEV in info.dimensions:
                    self._vars3Di_selection.AddArray(name)

        self._vars2D_selection.DisableAllArrays()
        self._vars3Dm_selection.DisableAllArrays()
        self._vars3Di_selection.DisableAllArrays()

        if 'time' in vardata.variables:
            self._time_steps = list(vardata['time'][:].data.flatten())

    def SetDataFileName(self, fname):
        if fname and fname != "None" and fname != self._data_filename:
            self._data_filename = fname
            self._clear()
            self._populate_metadata()
            self.Modified()

    def SetConnFileName(self, fname):
        if fname != self._conn_filename:
            self._conn_filename = fname
            self._clear()
            if self._data_filename:
                self._populate_metadata()
            self.Modified()

    def GetTimestepValues(self):
        return self._time_steps

    def Get2DDataArrays(self):
        return self._vars2D_selection

    def Get3DmDataArrays(self):
        return self._vars3Dm_selection

    def Get3DiDataArrays(self):
        return self._vars3Di_selection

    def RequestInformation(self, request, inInfo, outInfo):
        executive = self.GetExecutive()
        for i in range(3):
            port = outInfo.GetInformationObject(i)
            port.Remove(executive.TIME_STEPS())
            port.Remove(executive.TIME_RANGE())
            if self._time_steps:
                for t in self._time_steps:
                    port.Append(executive.TIME_STEPS(), t)
                port.Append(executive.TIME_RANGE(), self._time_steps[0])
                port.Append(executive.TIME_RANGE(), self._time_steps[-1])
        return 1

    def _get_time_index(self, outInfo, port_index):
        executive = self.GetExecutive()
        time_info = outInfo.GetInformationObject(port_index)
        if time_info.Has(executive.UPDATE_TIME_STEP()) and len(self._time_steps) > 1:
            time = time_info.Get(executive.UPDATE_TIME_STEP())
            for i, t in enumerate(self._time_steps):
                if time <= t:
                    return i
        return 0

    def RequestData(self, request, inInfo, outInfo):
        if not self._conn_filename or not self._data_filename:
            print_error("Data file or connectivity file not provided!")
            return 0

        if not _has_deps:
            print_error("Required Python module 'netCDF4' or 'numpy' missing!")
            return 0

        meshdata = self._get_mesh_dataset()
        vardata = self._get_var_dataset()

        if not self._data_horizontal_dim:
            print_error("Could not identify horizontal dimension")
            return 0

        lat_var, lon_var = find_lat_lon_vars(meshdata)
        if not lat_var or not lon_var:
            print_error("Could not find lat/lon variables in connectivity file")
            return 0

        lat = meshdata[lat_var][:].data.flatten()
        lon = meshdata[lon_var][:].data.flatten()
        ncells2D = meshdata.dimensions[self._horizontal_dim].size

        executive = self.GetExecutive()
        from_port = request.Get(executive.FROM_OUTPUT_PORT())
        time_idx = self._get_time_index(outInfo, from_port)

        # Output 0: 2D surface data
        output2D = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 0))
        vtk_coords, cell_types, cell_array = build_2d_geometry(lat, lon, ncells2D)
        output2D.SetPoints(vtk_coords)
        output2D.VTKObject.SetCells(cell_types, cell_array)

        for name, varmeta in self._variables.items():
            if self._vars2D_selection.ArrayIsEnabled(name):
                data = load_variable_data(vardata, varmeta, time_idx)
                output2D.CellData.append(data, name)

        # Output 1: 3D middle layer (lev)
        try:
            lev = FindSpecialVariable(vardata, AtmosphereConstants.LEV, AtmosphereConstants.HYAM, AtmosphereConstants.HYBM)
            if lev is not None:
                output3Dm = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 1))
                vtk_coords, cell_types, cell_array = build_3d_geometry(lat, lon, lev, ncells2D)
                output3Dm.SetPoints(vtk_coords)
                output3Dm.VTKObject.SetCells(cell_types, cell_array)

                for name, varmeta in self._variables.items():
                    if self._vars3Dm_selection.ArrayIsEnabled(name):
                        data = load_variable_data(vardata, varmeta, time_idx)
                        output3Dm.CellData.append(data, name)

                output3Dm.FieldData.append(len(lev), "numlev")
                output3Dm.FieldData.append(lev, "lev")
        except Exception as e:
            print_error(f"Error processing middle layer variables: {e}")

        # Output 2: 3D interface layer (ilev)
        try:
            ilev = FindSpecialVariable(vardata, AtmosphereConstants.ILEV, AtmosphereConstants.HYAI, AtmosphereConstants.HYBI)
            if ilev is not None:
                output3Di = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 2))
                vtk_coords, cell_types, cell_array = build_3d_geometry(lat, lon, ilev, ncells2D)
                output3Di.SetPoints(vtk_coords)
                output3Di.VTKObject.SetCells(cell_types, cell_array)

                for name, varmeta in self._variables.items():
                    if self._vars3Di_selection.ArrayIsEnabled(name):
                        data = load_variable_data(vardata, varmeta, time_idx)
                        output3Di.CellData.append(data, name)

                output3Di.FieldData.append(len(ilev), "numilev")
                output3Di.FieldData.append(ilev, "ilev")
        except Exception as e:
            print_error(f"Error processing interface layer variables: {e}")

        return 1


# ==============================================================================
# AtmosphereReader2D - Single output reader with level slicing for 3D data
# ==============================================================================

class AtmosphereReader2D(VTKPythonAlgorithmBase):
    """
    Single-output NetCDF reader with vertical level slicing for 3D data.

    This reader produces a single 2D mesh output, projecting 3D atmospheric data
    onto a 2D surface by selecting specific vertical level indices. This is useful
    for creating lightweight 2D visualizations of atmospheric layers without the
    memory overhead of loading full 3D data.

    All selected variables (2D and 3D) are output on the same 2D mesh. For 3D
    variables, data is extracted at the specified level index and placed on the
    2D cell array.

    Parameters
    ----------
    FileName1 : str
        Path to the NetCDF data file containing atmospheric variables.
    FileName2 : str
        Path to the NetCDF connectivity file containing mesh topology.
    Middle Layer : int
        Zero-based index for the middle level (lev) to extract. Default is 0
        (top of atmosphere in pressure coordinates).
    Interface Layer : int
        Zero-based index for the interface level (ilev) to extract. Default is 0.

    Output
    ------
    Port 0 : vtkUnstructuredGrid
        2D mesh with quad cells containing:
        - All selected 2D variables as cell data
        - Selected 3D-lev variables sliced at the Middle Layer index
        - Selected 3D-ilev variables sliced at the Interface Layer index
        - Field data arrays 'lev' and 'ilev' with full vertical coordinate arrays

    Notes
    -----
    - The geometry is cached and reused when only the time step or selected
      variables change, improving performance for time animation.
    - Level indices are clamped to valid ranges; warnings are printed for
      out-of-bounds values.
    - Unlike AtmosphereReader3D, this reader does not separate variables by type into
      different outputs - all data appears on a single 2D mesh.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = AtmosphereReader2D()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.SetMiddleLayer(30)  # Extract level 30
        reader.Get3DmDataArrays().EnableArray('T')
        reader.Update()
    """

    def __init__(self):
        VTKPythonAlgorithmBase.__init__(
            self, nInputPorts=0, nOutputPorts=1, outputType='vtkUnstructuredGrid')

        self._data_filename = None
        self._conn_filename = None
        self._dirty = True
        self._time_steps = []

        self._lev_index = 0
        self._ilev_index = 0

        self._horizontal_dim = None
        self._data_horizontal_dim = None
        self._dimensions = {}
        self._variables = {}

        self._vars2D_selection = vtkDataArraySelection()
        self._vars3Dm_selection = vtkDataArraySelection()
        self._vars3Di_selection = vtkDataArraySelection()

        for sel in [self._vars2D_selection, self._vars3Dm_selection, self._vars3Di_selection]:
            sel.AddObserver("ModifiedEvent", createModifiedCallback(self))

        self._mesh_dataset = None
        self._var_dataset = None
        self._cached_mesh_filename = None
        self._cached_var_filename = None

        self._cached_points = None
        self._cached_cell_types = None
        self._cached_cell_array = None
        self._cached_ncells = None
        self._area_var = None

    def __del__(self):
        self._close_datasets()

    def _close_datasets(self):
        for ds in [self._mesh_dataset, self._var_dataset]:
            if ds is not None:
                try:
                    ds.close()
                except Exception:
                    pass
        self._mesh_dataset = None
        self._var_dataset = None

    def _get_mesh_dataset(self):
        if self._conn_filename != self._cached_mesh_filename or self._mesh_dataset is None:
            if self._mesh_dataset is not None:
                try:
                    self._mesh_dataset.close()
                except Exception:
                    pass
            self._mesh_dataset = netCDF4.Dataset(self._conn_filename, "r")
            self._cached_mesh_filename = self._conn_filename
        return self._mesh_dataset

    def _get_var_dataset(self):
        if self._data_filename != self._cached_var_filename or self._var_dataset is None:
            if self._var_dataset is not None:
                try:
                    self._var_dataset.close()
                except Exception:
                    pass
            self._var_dataset = netCDF4.Dataset(self._data_filename, "r")
            self._cached_var_filename = self._data_filename
        return self._var_dataset

    def _clear(self):
        self._variables.clear()
        self._dimensions.clear()
        self._time_steps.clear()
        self._horizontal_dim = None
        self._data_horizontal_dim = None
        self._area_var = None

    def _clear_geometry_cache(self):
        self._cached_points = None
        self._cached_cell_types = None
        self._cached_cell_array = None
        self._cached_ncells = None

    def _populate_metadata(self):
        if self._data_filename is None or self._conn_filename is None:
            return

        meshdata = self._get_mesh_dataset()
        vardata = self._get_var_dataset()

        self._horizontal_dim, self._data_horizontal_dim, _ = identify_horizontal_dim(meshdata, vardata)
        if not self._data_horizontal_dim:
            print_warning("Could not match horizontal dimension in data file")
            return

        self._vars2D_selection.RemoveAllArrays()
        self._vars3Dm_selection.RemoveAllArrays()
        self._vars3Di_selection.RemoveAllArrays()

        for dim_name, dim_obj in vardata.dimensions.items():
            dim_meta = DimMeta(dim_name, dim_obj.size)
            if dim_name in vardata.variables:
                dim_meta.update_from_variable(vardata.variables[dim_name])
                try:
                    dim_meta.data = vardata[dim_name][:].data
                except Exception:
                    pass
            self._dimensions[dim_name] = dim_meta

        for name, info in vardata.variables.items():
            if self._data_horizontal_dim not in info.dimensions:
                continue

            varmeta = VarMeta(name, info, self._data_horizontal_dim)
            self._variables[name] = varmeta

            if len(info.dimensions) == 1 and 'area' in name.lower():
                self._area_var = varmeta
                continue

            ndims = len(info.dimensions)
            if ndims == 2:
                self._vars2D_selection.AddArray(name)
            elif ndims == 3:
                if AtmosphereConstants.LEV in info.dimensions:
                    self._vars3Dm_selection.AddArray(name)
                elif AtmosphereConstants.ILEV in info.dimensions:
                    self._vars3Di_selection.AddArray(name)

        self._vars2D_selection.DisableAllArrays()
        self._vars3Dm_selection.DisableAllArrays()
        self._vars3Di_selection.DisableAllArrays()

        if 'time' in vardata.variables:
            self._time_steps = list(vardata['time'][:].data.flatten())

    def SetDataFileName(self, fname):
        if fname and fname != "None" and fname != self._data_filename:
            self._data_filename = fname
            self._dirty = True
            self._clear()
            self._populate_metadata()
            self.Modified()

    def SetConnFileName(self, fname):
        if fname != self._conn_filename:
            self._conn_filename = fname
            self._dirty = True
            self._clear()
            self._clear_geometry_cache()
            if self._data_filename:
                self._populate_metadata()
            self.Modified()

    def SetMiddleLayer(self, lev):
        if self._lev_index != lev:
            self._lev_index = lev
            self.Modified()

    def SetInterfaceLayer(self, ilev):
        if self._ilev_index != ilev:
            self._ilev_index = ilev
            self.Modified()

    def GetTimestepValues(self):
        return self._time_steps

    def Get2DDataArrays(self):
        return self._vars2D_selection

    def Get3DmDataArrays(self):
        return self._vars3Dm_selection

    def Get3DiDataArrays(self):
        return self._vars3Di_selection

    def RequestInformation(self, request, inInfo, outInfo):
        executive = self.GetExecutive()
        port = outInfo.GetInformationObject(0)
        port.Remove(executive.TIME_STEPS())
        port.Remove(executive.TIME_RANGE())
        if self._time_steps:
            for t in self._time_steps:
                port.Append(executive.TIME_STEPS(), t)
            port.Append(executive.TIME_RANGE(), self._time_steps[0])
            port.Append(executive.TIME_RANGE(), self._time_steps[-1])
        return 1

    def _get_time_index(self, outInfo):
        executive = self.GetExecutive()
        time_info = outInfo.GetInformationObject(0)
        if time_info.Has(executive.UPDATE_TIME_STEP()) and len(self._time_steps) > 1:
            time = time_info.Get(executive.UPDATE_TIME_STEP())
            for i, t in enumerate(self._time_steps):
                if time <= t:
                    return i
        return 0

    def _build_geometry(self, meshdata):
        if self._cached_points is not None:
            return

        lat_var, lon_var = find_lat_lon_vars(meshdata)
        if not lat_var or not lon_var:
            print_error("Could not find lat/lon variables in connectivity file")
            return

        lat = meshdata[lat_var][:].data.flatten()
        lon = meshdata[lon_var][:].data.flatten()
        ncells = meshdata.dimensions[self._horizontal_dim].size

        self._cached_points, self._cached_cell_types, self._cached_cell_array = \
            build_2d_geometry(lat, lon, ncells)
        self._cached_ncells = ncells

    def _load_sliced_variable(self, vardata, varmeta, time_idx, level_idx, ncells):
        try:
            if varmeta.transpose:
                data = vardata[varmeta.name][:].data[time_idx].transpose().flatten()
            else:
                data = vardata[varmeta.name][:].data[time_idx].flatten()

            start = level_idx * ncells
            end = start + ncells
            data = data[start:end]

            return np.where(data == varmeta.fillval, np.nan, data)
        except Exception as e:
            print_error(f"Error loading variable {varmeta.name}: {e}")
            return np.array([])

    def RequestData(self, request, inInfo, outInfo):
        if not self._conn_filename or not self._data_filename:
            print_error("Data file or connectivity file not provided!")
            return 0

        if not _has_deps:
            print_error("Required Python module 'netCDF4' or 'numpy' missing!")
            return 0

        meshdata = self._get_mesh_dataset()
        vardata = self._get_var_dataset()

        if not self._data_horizontal_dim:
            print_error("Could not identify horizontal dimension")
            return 0

        self._build_geometry(meshdata)
        if self._cached_points is None:
            return 0

        ncells = self._cached_ncells
        time_idx = self._get_time_index(outInfo)

        output = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 0))
        output.SetPoints(self._cached_points)
        output.VTKObject.SetCells(self._cached_cell_types, self._cached_cell_array)
        self._dirty = False

        # Load 2D variables
        for name, varmeta in self._variables.items():
            if self._vars2D_selection.ArrayIsEnabled(name):
                data = load_variable_data(vardata, varmeta, time_idx)
                output.CellData.append(data, name)

        # Load 3D middle layer (lev) variables - sliced
        try:
            lev = FindSpecialVariable(vardata, AtmosphereConstants.LEV, AtmosphereConstants.HYAM, AtmosphereConstants.HYBM)
            if lev is not None:
                lev_dim = self._dimensions.get(AtmosphereConstants.LEV)
                if lev_dim and self._lev_index >= lev_dim.size:
                    print_warning(f"Middle layer index {self._lev_index} exceeds max {lev_dim.size - 1}")

                for name, varmeta in self._variables.items():
                    if self._vars3Dm_selection.ArrayIsEnabled(name):
                        data = self._load_sliced_variable(vardata, varmeta, time_idx, self._lev_index, ncells)
                        output.CellData.append(data, name)

                output.FieldData.append(lev, "lev")
        except Exception as e:
            print_error(f"Error processing middle layer variables: {e}")

        # Load 3D interface layer (ilev) variables - sliced
        try:
            ilev = FindSpecialVariable(vardata, AtmosphereConstants.ILEV, AtmosphereConstants.HYAI, AtmosphereConstants.HYBI)
            if ilev is not None:
                ilev_dim = self._dimensions.get(AtmosphereConstants.ILEV)
                if ilev_dim and self._ilev_index >= ilev_dim.size:
                    print_warning(f"Interface layer index {self._ilev_index} exceeds max {ilev_dim.size - 1}")

                for name, varmeta in self._variables.items():
                    if self._vars3Di_selection.ArrayIsEnabled(name):
                        data = self._load_sliced_variable(vardata, varmeta, time_idx, self._ilev_index, ncells)
                        output.CellData.append(data, name)

                output.FieldData.append(ilev, "ilev")
        except Exception as e:
            print_error(f"Error processing interface layer variables: {e}")

        return 1


# ==============================================================================
# AtmosphereVolumize - Convert 3D quad layers to hexahedral cells
# ==============================================================================

class AtmosphereVolumize(VTKPythonAlgorithmBase):
    """
    Convert stacked 2D quad layers into 3D hexahedral cells.

    This filter takes the 3D output from AtmosphereReader3D (Port 1 or Port 2) which
    consists of stacked 2D quad layers at each vertical level, and converts
    them into true 3D hexahedral (brick) cells. This enables proper volume
    rendering and 3D visualization of atmospheric data.

    The filter creates hexahedra by connecting adjacent vertical layers:
    each hex cell spans from level N to level N+1. Cell data is averaged
    between the two layers to produce values for the hex cells.

    Input
    -----
    vtkUnstructuredGrid
        3D mesh from AtmosphereReader3D with:
        - Stacked 2D quad cells at each vertical level
        - 'lev' field data array containing vertical level values
        - Cell data arrays with atmospheric variables

    Output
    ------
    vtkUnstructuredGrid
        3D mesh with VTK_HEXAHEDRON cells where:
        - Number of hex cells = (num_levels - 1) * num_2D_cells
        - Cell data is averaged between adjacent input layers
        - Point data is preserved unchanged

    Notes
    -----
    - The output has one fewer vertical layer than the input since hexahedra
      span between adjacent levels.
    - The filter applies vtkCleanUnstructuredGrid to remove duplicate points
      and optimize the mesh.
    - Cell data averaging assumes linear interpolation between layers;
      this may not be physically accurate for all variables.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = AtmosphereReader3D()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.Get3DmDataArrays().EnableArray('T')
        reader.Update()

        # Get 3D-lev output (Port 1) and volumize
        volumizer = AtmosphereVolumize(Input=OutputPort(reader, 1))
        volumizer.Update()
    """

    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkUnstructuredGrid")

    def RequestDataObject(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        assert inData is not None
        if outData is None or (not outData.IsA(inData.GetClassName())):
            outData = inData.NewInstance()
            outInfo.GetInformationObject(0).Set(outData.DATA_OBJECT(), outData)
        return super().RequestDataObject(request, inInfo, outInfo)

    def RequestData(self, request, inInfo, outInfo):
        if not _has_deps:
            print_error("Required Python module 'numpy' missing!")
            return 0

        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)

        inp = dsa.WrapDataObject(inData)
        output = dsa.WrapDataObject(outData)

        numpoints = inp.GetNumberOfPoints()
        numcells_i = inp.GetNumberOfCells()

        zCoords = inp.FieldData.GetArray("lev")
        if zCoords is None:
            print_error("Input must have 'lev' field data from AtmosphereReader3D 3D output")
            return 0

        numLevs = len(zCoords)
        numpoints2D = np.int64(numpoints / numLevs)
        numcells2D = np.int64(numcells_i / numLevs)
        hexstacksize = numLevs - 1
        numhexes = np.int64(numcells2D * hexstacksize)

        # Build hex connectivity
        hexconn = np.empty((hexstacksize, numcells2D * 8), dtype=np.int64)
        for i in range(hexstacksize):
            lower = np.arange(i * numpoints2D, (i + 1) * numpoints2D, dtype=np.int64).reshape(numcells2D, 4).transpose()
            upper = np.arange((i + 1) * numpoints2D, (i + 2) * numpoints2D, dtype=np.int64).reshape(numcells2D, 4).transpose()
            conn = np.append(lower, upper, axis=0).flatten('F')
            hexconn[i] = conn
        hexconn = hexconn.flatten()

        output.SetPoints(inp.GetPoints())

        cellTypes = np.empty(numhexes, dtype=np.uint8)
        cellTypes.fill(vtkConstants.VTK_HEXAHEDRON)
        cellTypes = numpy_support.numpy_to_vtk(
            num_array=cellTypes, deep=True, array_type=vtkConstants.VTK_UNSIGNED_CHAR)

        offsets = np.arange(0, (8 * numhexes) + 1, 8, dtype=np.int64)
        offsets = numpy_support.numpy_to_vtk(
            num_array=offsets, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

        cells = numpy_support.numpy_to_vtk(
            num_array=hexconn, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

        cellArray = vtkCellArray()
        cellArray.SetData(offsets, cells)
        output.VTKObject.SetCells(cellTypes, cellArray)

        # Average cell data between adjacent layers
        numVars = inData.GetCellData().GetNumberOfArrays()
        for i in range(numVars):
            varname = inData.GetCellData().GetArray(i).GetName()
            vardata = np.array(inData.GetCellData().GetArray(i))
            outvardata = np.empty((hexstacksize, numcells2D))
            for j in range(hexstacksize):
                lower = vardata[j * numcells2D:(j + 1) * numcells2D]
                upper = vardata[(j + 1) * numcells2D:(j + 2) * numcells2D]
                outvardata[j] = (upper + lower) / 2
            output.CellData.append(outvardata.flatten(), varname)

        # Copy point data
        numVars = inData.GetPointData().GetNumberOfArrays()
        for i in range(numVars):
            varname = inData.GetPointData().GetArray(i).GetName()
            vardata = np.array(inData.GetPointData().GetArray(i))
            output.PointData.append(vardata, varname)

        # Clean the grid
        cleaner = uGridFilter()
        cleaner.SetInputData(output.VTKObject)
        cleaner.Update()
        outData.ShallowCopy(cleaner.GetOutput())

        return 1


# ==============================================================================
# AtmosphereExtractSlices - Extract a range of vertical slices from 3D data
# ==============================================================================

class AtmosphereExtractSlices(VTKPythonAlgorithmBase):
    """
    Extract a contiguous range of vertical slices from 3D atmosphere data.

    This filter extracts a subset of vertical levels from 3D atmosphere output,
    useful for focusing on specific atmospheric regions such as the troposphere,
    stratosphere, or boundary layer without processing the entire vertical column.

    The extracted mesh maintains the same structure as the input (stacked 2D
    quad layers) but contains only the selected range of levels.

    Parameters
    ----------
    MinMaxPlanes : tuple of int
        Two-element tuple (min, max) specifying the inclusive range of level
        indices to extract. Level 0 is typically the top of atmosphere.
        Default is (0, 0) which extracts only the first level.

    Input
    -----
    vtkUnstructuredGrid
        3D mesh from AtmosphereReader3D with:
        - Stacked 2D quad cells at each vertical level
        - 'lev' field data array containing vertical level values

    Output
    ------
    vtkUnstructuredGrid
        Subset of input mesh containing only levels in [min, max] range:
        - Points and cells for selected levels only
        - Cell data sliced to selected levels
        - Point data sliced to selected levels
        - Updated 'lev' and 'numlev' field data reflecting extracted range

    Notes
    -----
    - The min/max values are clamped to valid ranges (0 to num_levels-1).
    - Setting min > max will result in an error.
    - This filter is useful before applying AtmosphereVolumize to reduce memory usage.
    - VTK arrays starting with 'vtk' are excluded from the output to avoid
      copying internal metadata arrays.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = AtmosphereReader3D()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.Get3DmDataArrays().EnableArray('T')
        reader.Update()

        # Extract troposphere (levels 50-71, assuming 72 total levels)
        extractor = AtmosphereExtractSlices(Input=OutputPort(reader, 1))
        extractor.SetMaxPlane(50, 71)
        extractor.Update()
    """

    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkUnstructuredGrid")
        self._min = 0
        self._max = 0

    def RequestDataObject(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        assert inData is not None
        if outData is None or (not outData.IsA(inData.GetClassName())):
            outData = inData.NewInstance()
            outInfo.GetInformationObject(0).Set(outData.DATA_OBJECT(), outData)
        return super().RequestDataObject(request, inInfo, outInfo)

    def SetMaxPlane(self, min_val, max_val):
        if min_val > max_val:
            print_error("Plane min cannot exceed plane max")
            return
        if self._max != max_val:
            self._max = max_val
            self.Modified()
        if self._min != min_val:
            self._min = min_val
            self.Modified()

    def RequestData(self, request, inInfo, outInfo):
        if not _has_deps:
            print_error("Required Python module 'numpy' missing!")
            return 0

        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)

        inp = dsa.WrapDataObject(inData)
        output = dsa.WrapDataObject(outData)

        numPoints = inp.GetNumberOfPoints()
        numCells = inp.GetNumberOfCells()

        zCoords = inp.FieldData.GetArray("lev")
        if zCoords is None:
            print_error("Input must have 'lev' field data from AtmosphereReader3D 3D output")
            return 0

        numLevs = len(zCoords)
        numPoints2D = np.int64(numPoints / numLevs)
        numCells2D = np.int64(numCells / numLevs)

        pMin = max(self._min, 0)
        pMax = min(self._max, numLevs - 1)
        if pMin > pMax:
            print_error("Error in interpreting min and max planes")
            return 0

        pStart = pMin * numPoints2D
        pEnd = (pMax + 1) * numPoints2D
        cStart = pMin * numCells2D
        cEnd = (pMax + 1) * numCells2D
        numPlanes = (pMax - pMin) + 1

        # Extract points
        inpPoints = inp.GetPoints()
        outPoints = inpPoints[pStart:pEnd]
        output.SetPoints(outPoints)

        # Build cells
        cellTypes = np.empty(numCells2D * numPlanes, dtype=np.uint8)
        cellTypes.fill(vtkConstants.VTK_QUAD)
        cellTypes = numpy_support.numpy_to_vtk(
            num_array=cellTypes, deep=True, array_type=vtkConstants.VTK_UNSIGNED_CHAR)

        offsets = np.arange(0, (4 * numCells2D * numPlanes) + 1, 4, dtype=np.int64)
        offsets = numpy_support.numpy_to_vtk(
            num_array=offsets, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

        cells = np.arange(numCells2D * numPlanes * 4, dtype=np.int64)
        cells = numpy_support.numpy_to_vtk(
            num_array=cells, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

        cellArray = vtkCellArray()
        cellArray.SetData(offsets, cells)
        output.VTKObject.SetCells(cellTypes, cellArray)

        # Extract cell data
        numVars = inData.GetCellData().GetNumberOfArrays()
        for i in range(numVars):
            varname = inData.GetCellData().GetArray(i).GetName()
            if varname.startswith("vtk"):
                continue
            inpVardata = np.array(inData.GetCellData().GetArray(i))
            outVardata = inpVardata[cStart:cEnd]
            output.CellData.append(outVardata, varname)

        # Extract point data
        numVars = inData.GetPointData().GetNumberOfArrays()
        for i in range(numVars):
            varname = inData.GetPointData().GetArray(i).GetName()
            if varname.startswith("vtk"):
                continue
            inpVardata = np.array(inData.GetPointData().GetArray(i))
            outVardata = inpVardata[pStart:pEnd]
            output.PointData.append(outVardata, varname)

        # Update field data
        output.FieldData.append(numPlanes, "numlev")
        output.FieldData.append(zCoords[pMin:pMax + 1], "lev")

        return 1


# ==============================================================================
# AtmosphereSphere - Project data onto spherical coordinates
# ==============================================================================

class AtmosphereSphere(VTKPythonAlgorithmBase):
    """
    Project lat/lon data onto a 3D sphere for globe visualization.

    This filter transforms points from geographic coordinates (longitude as X,
    latitude as Y, vertical level as Z) into spherical coordinates for
    rendering data on a 3D globe. It supports vertical exaggeration through
    a scale factor to make atmospheric layers visible.

    The transformation maps:
    - Longitude -> azimuthal angle (theta)
    - Latitude -> polar angle (phi), measured from north pole
    - Z-coordinate -> radial distance from sphere center

    Parameters
    ----------
    Data Layer : bool
        If True, adds a small offset (1 unit) to the radius. This is useful
        when overlaying data on a separate sphere geometry to prevent
        z-fighting artifacts. Default is False.
    Scale : float
        Vertical exaggeration factor for the Z-coordinate. Higher values make
        vertical structure more visible. Default is 1.0. A value of 0 places
        all points on the sphere surface.

    Input
    -----
    vtkUnstructuredGrid or vtkPolyData
        Mesh with points in geographic coordinates:
        - X: Longitude in degrees (-180 to 180 or 0 to 360)
        - Y: Latitude in degrees (-90 to 90)
        - Z: Vertical level (e.g., pressure in hPa, or index)

    Output
    ------
    vtkUnstructuredGrid
        Same topology as input but with points transformed to spherical
        coordinates. Point data and cell data are preserved unchanged.

    Notes
    -----
    - The default sphere radius is 2000 units, designed for visualization
      with typical atmospheric data scales.
    - For 2D surface data (Z=0), all points are placed exactly on the sphere.
    - For 3D data, higher Z values (e.g., higher pressure = lower altitude)
      result in smaller radii, placing those points closer to the sphere center.
    - vtkPolyData input is automatically converted to vtkUnstructuredGrid.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = AtmosphereReader3D()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.Get2DDataArrays().EnableArray('TREFHT')
        reader.Update()

        # Project 2D output onto sphere
        sphere = AtmosphereSphere(Input=OutputPort(reader, 0))
        sphere.SetDataLayer(True)  # Slight offset for layering
        sphere.Update()

        # For 3D data with vertical exaggeration
        sphere3d = AtmosphereSphere(Input=OutputPort(reader, 1))
        sphere3d.SetScalingFactor(10.0)  # Exaggerate vertical structure
        sphere3d.Update()
    """

    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkUnstructuredGrid")
        self._is_data_layer = False
        self._radius = 2000
        self._scale = 1.0

    def SetDataLayer(self, is_data):
        if self._is_data_layer != is_data:
            self._is_data_layer = is_data
            self.Modified()

    def SetScalingFactor(self, scale):
        if self._scale != float(scale):
            self._scale = float(scale)
            self.Modified()

    def RequestDataObject(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        assert inData is not None
        if outData is None or (not outData.IsA(inData.GetClassName())):
            outData = inData.NewInstance()
            outInfo.GetInformationObject(0).Set(outData.DATA_OBJECT(), outData)
        return super().RequestDataObject(request, inInfo, outInfo)

    def RequestData(self, request, inInfo, outInfo):
        if not _has_deps:
            print_error("Required Python module 'numpy' missing!")
            return 0

        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)

        # Handle PolyData input by converting to UnstructuredGrid
        if inData.IsA('vtkPolyData'):
            afilter = vtkAppendFilter()
            afilter.AddInputData(inData)
            afilter.Update()
            outData.DeepCopy(afilter.GetOutput())
        else:
            outData.DeepCopy(inData)

        inWrap = dsa.WrapDataObject(inData)
        outWrap = dsa.WrapDataObject(outData)

        inPoints = np.array(inWrap.Points)
        max_z = np.max(inPoints[:, 2])

        # Offset radius for data layer
        radius = (self._radius + 1) if self._is_data_layer else self._radius

        # Transform points to spherical coordinates
        outPoints = np.array([
            process_point_to_sphere(pt, max_z, radius, self._scale)
            for pt in inPoints
        ])

        vtk_coords = vtkPoints()
        vtk_coords.SetData(numpy_support.numpy_to_vtk(
            outPoints, deep=True, array_type=vtkConstants.VTK_FLOAT))
        outWrap.SetPoints(vtk_coords)

        return 1
