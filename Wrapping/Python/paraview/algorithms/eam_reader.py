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
EAMDataReader
    Multi-output reader that produces three separate outputs:
    - Port 0: 2D surface variables (e.g., surface temperature, precipitation)
    - Port 1: 3D variables on middle levels (lev) as hexahedral volume mesh
    - Port 2: 3D variables on interface levels (ilev) as hexahedral volume mesh

Filters
-------
EAMProjectToSphere
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
2. Choose "EAM Data Reader"
3. Set the connectivity file path in the Properties panel
4. Select variables to load from the array selection widgets
5. Apply to visualize

For spherical projection:
1. Apply EAMDataReader reader
2. Apply EAMProjectToSphere filter
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


def build_3d_volume_geometry(lat, lon, levels, ncells2d):
    """
    Build VTK geometry arrays for 3D volumetric mesh with hexahedral cells.

    Creates hexahedra by connecting adjacent vertical layers. Each hex cell
    spans from level N to level N+1, resulting in (num_levels - 1) layers
    of hexahedral cells.

    Parameters
    ----------
    lat : ndarray
        Flattened latitude coordinates for cell corners.
    lon : ndarray
        Flattened longitude coordinates for cell corners.
    levels : ndarray
        Vertical level coordinates.
    ncells2d : int
        Number of 2D cells per horizontal layer.

    Returns
    -------
    vtk_coords : vtkPoints
        Points for all vertical levels.
    cell_types : vtkUnsignedCharArray
        Cell type array (all VTK_HEXAHEDRON).
    cell_array : vtkCellArray
        Cell connectivity array.
    """
    nlev = len(levels)
    npts_per_level = len(lat)

    # Build 3D coordinates for all levels
    coords = np.empty((nlev, npts_per_level, 3), dtype=np.float64)
    for i, z in enumerate(levels):
        coords[i, :, 0] = lon
        coords[i, :, 1] = lat
        coords[i, :, 2] = z
    coords = coords.reshape(nlev * npts_per_level, 3)

    vtk_coords = vtkPoints()
    vtk_coords.SetData(dsa.numpyTovtkDataArray(coords))

    # Build hexahedral cells connecting adjacent levels
    # Number of hex layers = nlev - 1
    hex_layers = nlev - 1
    numhexes = ncells2d * hex_layers

    # Build hex connectivity efficiently using numpy
    # Each hex connects 4 points from lower level to 4 points from upper level
    hexconn = np.empty((hex_layers, ncells2d * 8), dtype=np.int64)
    for i in range(hex_layers):
        # Lower level point indices (4 corners per cell)
        lower = np.arange(i * npts_per_level, (i + 1) * npts_per_level, dtype=np.int64)
        lower = lower.reshape(ncells2d, 4).transpose()
        # Upper level point indices (4 corners per cell)
        upper = np.arange((i + 1) * npts_per_level, (i + 2) * npts_per_level, dtype=np.int64)
        upper = upper.reshape(ncells2d, 4).transpose()
        # Combine: 4 lower + 4 upper = 8 points per hex
        conn = np.append(lower, upper, axis=0).flatten('F')
        hexconn[i] = conn
    hexconn = hexconn.flatten()

    cell_types = np.empty(numhexes, dtype=np.uint8)
    cell_types.fill(vtkConstants.VTK_HEXAHEDRON)
    cell_types = numpy_support.numpy_to_vtk(
        num_array=cell_types, deep=True, array_type=vtkConstants.VTK_UNSIGNED_CHAR)

    offsets = np.arange(0, (8 * numhexes) + 1, 8, dtype=np.int64)
    offsets = numpy_support.numpy_to_vtk(
        num_array=offsets, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

    cells = numpy_support.numpy_to_vtk(
        num_array=hexconn, deep=True, array_type=vtkConstants.VTK_ID_TYPE)

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


def load_variable_data_averaged(vardata, varmeta, time_idx, ncells2d, nlev):
    """
    Load 3D variable data and average between adjacent vertical levels.

    For hexahedral cells that span between levels N and N+1, the cell data
    is computed as the average of the values at those two levels.

    Parameters
    ----------
    vardata : netCDF4.Dataset
        NetCDF dataset containing the variable.
    varmeta : VarMeta
        Variable metadata.
    time_idx : int
        Time index to load.
    ncells2d : int
        Number of 2D cells per horizontal layer.
    nlev : int
        Number of vertical levels in the data.

    Returns
    -------
    ndarray
        Averaged cell data with shape (nlev-1) * ncells2d.
    """
    try:
        if varmeta.transpose:
            data = vardata[varmeta.name][:].data[time_idx].transpose().flatten()
        else:
            data = vardata[varmeta.name][:].data[time_idx].flatten()

        # Replace fill values with NaN
        data = np.where(data == varmeta.fillval, np.nan, data)

        # Average between adjacent levels
        hex_layers = nlev - 1
        averaged = np.empty(hex_layers * ncells2d, dtype=np.float64)

        for i in range(hex_layers):
            lower = data[i * ncells2d:(i + 1) * ncells2d]
            upper = data[(i + 1) * ncells2d:(i + 2) * ncells2d]
            averaged[i * ncells2d:(i + 1) * ncells2d] = (lower + upper) / 2.0

        return averaged
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
# EAMDataReader - Multi-output reader for 2D and 3D volumetric data
# ==============================================================================

class EAMDataReader(VTKPythonAlgorithmBase):
    """
    Multi-output NetCDF reader for E3SM/EAM atmospheric model data.

    This reader produces three separate outputs for different variable types:
    - Port 0 (2D): Surface variables with 2 dimensions (time, ncol) as quad mesh
    - Port 1 (3D-lev): Variables on middle levels as hexahedral volume mesh
    - Port 2 (3D-ilev): Variables on interface levels as hexahedral volume mesh

    The 3D outputs are volumized directly in the reader, producing hexahedral cells
    that span between adjacent vertical levels. This enables immediate volume
    rendering without requiring additional filters.

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
        3D volumetric mesh for middle-level (lev) variables with hexahedral cells.
        Cell data is averaged between adjacent levels. Contains 'lev' and 'numlev'
        in field data.
    Port 2 : vtkUnstructuredGrid
        3D volumetric mesh for interface-level (ilev) variables with hexahedral cells.
        Cell data is averaged between adjacent levels. Contains 'ilev' and 'numilev'
        in field data.

    Notes
    -----
    - Vertical coordinates are computed from hybrid sigma-pressure coefficients
      (hyam, hybm for lev; hyai, hybi for ilev) if explicit level arrays are missing.
    - The reader caches NetCDF file handles to avoid repeated file opens.
    - Variables are grouped by type in the ParaView UI for easy selection.
    - Time dimension is automatically detected and exposed for animation.
    - 3D outputs have (num_levels - 1) hexahedral cells in the vertical direction
      since each hex spans two adjacent levels.

    Example
    -------
    In ParaView Python shell::

        from paraview.simple import *
        reader = EAMDataReader()
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

        # Output 1: 3D middle layer (lev) - volumized hexahedral mesh
        try:
            lev = FindSpecialVariable(vardata, AtmosphereConstants.LEV, AtmosphereConstants.HYAM, AtmosphereConstants.HYBM)
            if lev is not None:
                nlev = len(lev)
                output3Dm = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 1))
                vtk_coords, cell_types, cell_array = build_3d_volume_geometry(lat, lon, lev, ncells2D)
                output3Dm.SetPoints(vtk_coords)
                output3Dm.VTKObject.SetCells(cell_types, cell_array)

                for name, varmeta in self._variables.items():
                    if self._vars3Dm_selection.ArrayIsEnabled(name):
                        data = load_variable_data_averaged(vardata, varmeta, time_idx, ncells2D, nlev)
                        output3Dm.CellData.append(data, name)

                output3Dm.FieldData.append(nlev - 1, "numlev")
                output3Dm.FieldData.append(lev, "lev")
        except Exception as e:
            print_error(f"Error processing middle layer variables: {e}")

        print(output3Dm.VTKObject)

        # Output 2: 3D interface layer (ilev) - volumized hexahedral mesh
        try:
            ilev = FindSpecialVariable(vardata, AtmosphereConstants.ILEV, AtmosphereConstants.HYAI, AtmosphereConstants.HYBI)
            if ilev is not None:
                nilev = len(ilev)
                output3Di = dsa.WrapDataObject(vtkUnstructuredGrid.GetData(outInfo, 2))
                vtk_coords, cell_types, cell_array = build_3d_volume_geometry(lat, lon, ilev, ncells2D)
                output3Di.SetPoints(vtk_coords)
                output3Di.VTKObject.SetCells(cell_types, cell_array)

                for name, varmeta in self._variables.items():
                    if self._vars3Di_selection.ArrayIsEnabled(name):
                        data = load_variable_data_averaged(vardata, varmeta, time_idx, ncells2D, nilev)
                        output3Di.CellData.append(data, name)

                output3Di.FieldData.append(nilev - 1, "numilev")
                output3Di.FieldData.append(ilev, "ilev")
        except Exception as e:
            print_error(f"Error processing interface layer variables: {e}")

        print(output3Di.VTKObject)

        return 1


# ==============================================================================
# EAMProjectToSphere - Project data onto spherical coordinates
# ==============================================================================

class EAMProjectToSphere(VTKPythonAlgorithmBase):
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
        reader = EAMDataReader()
        reader.FileName1 = '/path/to/data.nc'
        reader.FileName2 = '/path/to/connectivity.nc'
        reader.Get2DDataArrays().EnableArray('TREFHT')
        reader.Update()

        # Project 2D output onto sphere
        sphere = EAMProjectToSphere(Input=OutputPort(reader, 0))
        sphere.SetDataLayer(True)  # Slight offset for layering
        sphere.Update()

        # For 3D data with vertical exaggeration
        sphere3d = EAMProjectToSphere(Input=OutputPort(reader, 1))
        sphere3d.SetScalingFactor(10.0)  # Exaggerate vertical structure
        sphere3d.Update()
    """

    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkUnstructuredGrid")
        self._is_data_layer = False
        self._radius = 2000
        self._scale = 1.0

    def FillInputPortInformation(self, port, info):
        info.Set(self.INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet")
        return 1

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
