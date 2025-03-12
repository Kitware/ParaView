# Authors: Berk Geveci, Axel Huebl, Utkarsh Ayachit
#

from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from .. import print_error

import math

try:
    import numpy as np
    import openpmd_api as io

    _has_openpmd = True
except ImportError as ie:
    print_error(
        "Missing required Python modules/packages. Algorithms in this module may "
        "not work as expected! \n {0}".format(ie)
    )
    _has_openpmd = False


def createModifiedCallback(anobject):
    import weakref

    weakref_obj = weakref.ref(anobject)
    anobject = None

    def _markmodified(*args, **kwars):
        o = weakref_obj()
        if o is not None:
            o.Modified()

    return _markmodified

# Order of transposes relative to the layout of data
# For 3D data : in case of (z, y, x) ordering of axes
_3d = (2, 1, 0)
# For 2D data : in case of (z, x) ordering of axes
_2d = (1, 2, 0)
# For 1D data : in case data layout (z)
_1d = (2, 1, 0)

def mesh_needs_transpose(mesh):
    """VTK meshes are in order FortranArray[x,y,z].
    openPMD supports labeling nD arrays."""
    # the openPMD v1.0.*/1.1.* attribute dataOrder describes if metadata is to be inverted
    meta_data_in_C = mesh.data_order == "C"
    first_axis_label = mesh.axis_labels[0] if meta_data_in_C else mesh.axis_labels[-1]
    last_axis_label = mesh.axis_labels[-1] if meta_data_in_C else mesh.axis_labels[0]
    # common for 1D and 2D data in openPMD from accelerator physics: axes labeled as x-z and only z
    if (first_axis_label == "x" and last_axis_label == "z"):
        return False
    else:
        return True

class openPMDReader(VTKPythonAlgorithmBase):
    """A reader that reads openPMD format.

    openPMD is a meta-data format implemented in ADIOS1/2, HDF5, JSON and
    other hierarchical data formats.
    References:
    - https://github.com/openPMD
    - https://www.openPMD.org

    This class implements ... TODO ... VTKPythonAlgorithmBase

    In detail, we implement openPMD reading via the openPMD-api Python bindings.
    References:
    - https://openpmd-api.readthedocs.io
    - https://github.com/openPMD/openPMD-api
    """

    def __init__(self):
        """In the constructor, we initialize internal variables"""
        VTKPythonAlgorithmBase.__init__(
            self, nInputPorts=0, nOutputPorts=2, outputType="vtkPartitionedDataSet"
        )
        # path to ".pmd" one-line text file that holds the file name pattern for _series open
        # opening pattern. Reference on syntax in the file:
        # - https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#hierarchy-of-the-data-file
        # - https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#iterations-and-time-series
        # - https://openpmd-api.readthedocs.io/en/latest/_static/doxyhtml/classopen_p_m_d_1_1_series.html
        self._filename = None
        self._series = None  # openPMD-api's data series holder
        self._timemap = {}  # maps time steps (int) in _series to physical time (float)
        self._timevalues = None  # the float values in _timemap (TODO: deduplicate, get on the fly from _timemap)

        # instructs ParaView what data sources to expect
        # this registers observers and callbacks
        # in the UI: users can (un)select some groups in the data:
        # fields, particle species or individual particle properties
        from vtkmodules.vtkCommonCore import vtkDataArraySelection

        # 1D, 2D or 3D arrays in openPMD "meshes" aka "fields"
        # openPMD fields are regular grids
        self._arrayselection = vtkDataArraySelection()
        self._arrayselection.AddObserver("ModifiedEvent", createModifiedCallback(self))

        # openPMD species are groups of particle arrays
        self._speciesselection = vtkDataArraySelection()
        self._speciesselection.AddObserver(
            "ModifiedEvent", createModifiedCallback(self)
        )

        # each openPMD particle species is a struct (list) of arrays - like a dataframe (or table)
        # the array index (row of a table) corresponds to the same particle.
        # different particle species might have different array columns in that table
        self._particlearrayselection = vtkDataArraySelection()
        self._particlearrayselection.AddObserver(
            "ModifiedEvent", createModifiedCallback(self)
        )

    def _get_update_time(self, outInfo):
        """Finds the closest available time (float) to the requested time (float).

        TODO:
        - synchronize with the logic in openPMD-viewer
          https://github.com/openPMD/openPMD-viewer/pull/347
        - or move out of the reader into VTK itself, so all readers behave the same

        Parameters:

        outInfo: vktInformation
          Exchanges information between pipeline objects, e.g., between two filters.
          Contains the info that the consumer requests from the producer. Also, is
          updated with information by the producer once it is done.

          UPDATE_... are requests. Here, we request a new step for a time, e.g., to animate.

        Returns:

          The time (float) of the first time step available
          that is less than the requested time.
        """
        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

        executive = (
            vtkStreamingDemandDrivenPipeline  # UPDATE_TIME_STEP request is defined here
        )
        timevalues = self._timevalues

        # find closest time step (as a float time) that is less than requested time
        if timevalues is None or len(timevalues) == 0:
            return None
        elif outInfo.Has(executive.UPDATE_TIME_STEP()) and len(timevalues) > 0:
            utime = outInfo.Get(executive.UPDATE_TIME_STEP())
            dtime = timevalues[0]
            for atime in timevalues:
                if atime > utime:
                    return dtime
                else:
                    dtime = atime
            return dtime
        else:
            assert len(timevalues) > 0
            return timevalues[0]

    def _get_array_selection(self):
        """Which selected mesh arrays can be loaded.

        See vtkDataArraySelection for API.

        TODO: remove this, duplicate of GetDataArraySelection
        """
        return self._arrayselection

    def _get_particle_array_selection(self):
        """Which selected particle arrays can be loaded.

        See vtkDataArraySelection for API.

        TODO: remove this, duplicate of GetParticleArraySelection
        """
        return self._particlearrayselection

    def _get_species_selection(self):
        """Which selected particle species can be loaded.

        TODO: remove this, duplicate of GetSpeciesSelection
        """
        return self._speciesselection

    def SetFileName(self, name):
        """Specify filename for the file to read.

        Path to ".pmd" one-line text file that holds the file name pattern for _series open
        opening pattern. Reference on syntax in the file:
        - https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#hierarchy-of-the-data-file
        - https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#iterations-and-time-series
        - https://openpmd-api.readthedocs.io/en/latest/_static/doxyhtml/classopen_p_m_d_1_1_series.html
        """
        if self._filename != name:
            self._filename = name
            self._timevalues = None
            if self._series:
                self._series = None
            # updates the modified data source time to indicate a changed state in VTK
            # this triggers (partial/needed) pipeline updates when executed
            self.Modified()

    def GetTimestepValues(self):
        """Which physical time step values (as float) are available?

        Required by the UI: populates available time steps.
        """
        return self._timevalues()

    def GetDataArraySelection(self):
        """Which selected mesh arrays can be loaded.

        See vtkDataArraySelection for API.
        """
        return self._get_array_selection()

    def GetSpeciesSelection(self):
        """Which selected particle species can be loaded."""
        return self._get_species_selection()

    def GetParticleArraySelection(self):
        """Which selected particle arrays can be loaded.

        See vtkDataArraySelection for API.
        """
        return self._get_particle_array_selection()

    def FillOutputPortInformation(self, port, info):
        """Tells the pipeline which kind of data this source produces

        Declares two DATA_TYPE_NAME outputs:
        - vtkPartitionedDataSet: openPMD meshes
        - vtkPartitionedDataSetCollection: openPMD particle species

        Parameters:

        port: Int
          Output indices: 0 for meshes, 1 for particle species
        info: vtkInformation
          Gets updated to set two output types (DATA_TYPE_NAME).
        """
        from vtkmodules.vtkCommonDataModel import vtkDataObject

        if port == 0:
            info.Set(vtkDataObject.DATA_TYPE_NAME(), "vtkPartitionedDataSet")
        else:
            info.Set(vtkDataObject.DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection")
        return 1

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        """This is where we produce meta-data for the data pipeline.

        The meta-data we produce:
        - time information
        - mesh names
        - species names
        - particle array names per species

        Parameters:

        request:
          Not used.

        inInfoVec:
          Not used.

        outInfoVec: list of vtkInformation
          One per output port because they can have different meta-data.
          We populate this object.
        """
        global _has_openpmd
        if not _has_openpmd:
            print_error("Required Python module 'openpmd_api' missing!")
            return 0

        from vtkmodules.vtkCommonExecutionModel import (
            vtkStreamingDemandDrivenPipeline,
            vtkAlgorithm,
        )

        # clear the output
        # tell the pipeline it is a parallel reader
        executive = vtkStreamingDemandDrivenPipeline
        for i in (0, 1):  # loop over ports (meshes, particles)
            outInfo = outInfoVec.GetInformationObject(i)
            outInfo.Remove(executive.TIME_STEPS())
            outInfo.Remove(executive.TIME_RANGE())
            outInfo.Set(vtkAlgorithm.CAN_HANDLE_PIECE_REQUEST(), 1)

        # TODO: Why is this a string when it is None?
        if self._filename == "None":
            return 1

        # open the ".pmd" file that contains the pattern for io.Series(pattern)
        mfile = open(self._filename, "r")
        pattern = mfile.readlines()[0][0:-1]
        del mfile

        import os

        # Open the openPMD data series.
        # We cache the series to only parse I/O meta data from disk once.
        if not self._series:
            self._series = io.Series(
                os.path.dirname(self._filename) + "/" + pattern,
                io.Access_Type.read_only,
            )

        # This is how we get time values and arrays
        self._timemap = {}
        timevalues = []
        arrays = set()
        particles = set()
        species = set()
        # an openPMD iteration is a time step that contains
        # meshes and particle species
        for idx, iteration in self._series.iterations.items():
            # extract the time
            if callable(iteration.time):  # prior to openPMD-api 0.13.0
                time = iteration.time() * iteration.time_unit_SI()
            else:
                time = iteration.time * iteration.time_unit_SI
            timevalues.append(time)
            self._timemap[time] = idx

            # Update openPMD meshes, which are openPMD records.
            # An openPMD record is either an array or a list of arrays.
            # Reference:
            # - https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#scalar-vector-and-tensor-records
            # example: ["E", "B", "rho", "rho_electrons"]
            arrays.update([mesh_name for mesh_name, mesh in iteration.meshes.items()])

            # Update openPMD particle species.
            # An openPMD particle species is a list of openPMD records.
            species.update(
                # example: ["electrons", "protons", "carbons"]
                [species_name for species_name, _ in iteration.particles.items()]
            )
            # these are now all particles, prefixed by the species name.
            # different species often have different openPMD records associated with them.
            # the most common openPMD record they all have is named position.
            particles.update(
                # example: ["electrons_position", "electrons_momentum",
                #           "protons_position", "carbons_position", "carbons_ionizationLevel"]
                [
                    species_name + "_" + record_name
                    for species_name, species in iteration.particles.items()
                    for record_name, record in species.items()
                ]
            )

        # Populate the available meshes and particle species in the openPMD series:
        # this is used in the UI to provide a selection of meshes and particle species.
        for array in arrays:
            self._arrayselection.AddArray(array)
        for particle_array in particles:
            self._particlearrayselection.AddArray(particle_array)
        for species_name in species:
            self._speciesselection.AddArray(species_name)

        # make available the time steps and their corresponding physical time (float)
        # known. sets the time range to their min/max.
        timesteps = list(self._series.iterations)
        self._timevalues = timevalues
        if len(timevalues) > 0:
            for i in (0, 1):
                outInfo = outInfoVec.GetInformationObject(i)
                for t in timevalues:
                    outInfo.Append(executive.TIME_STEPS(), t)
                outInfo.Append(executive.TIME_RANGE(), timevalues[0])
                outInfo.Append(executive.TIME_RANGE(), timevalues[-1])
        return 1

    def _get_array_and_component(self, itr, name):
        """Differentiate scalar and vector/tensor openPMD meshes.

        Scalars have no components, e.g., a density field "rho".
        Vectors/tensors have components, e.g., the electric field E ("E_x", "E_y", "E_z").

        Parameters:

        itr: openPMD.Iteration
          The current iteration to inspect.
        name: openPMD record name of the mesh, e.g., "rho" or "E_x"

        Returns: tuple of strings
          (mesh_name, None) for scalars
          (mesh_name, component_name) for vector/tensors
        """
        for mesh_name, mesh in itr.meshes.items():
            if mesh_name == name:
                return (mesh_name, None)
            for comp_name, _ in mesh.items():
                if name == mesh_name + "_" + comp_name:
                    return (mesh_name, comp_name)
        return (None, None)

    def _get_particle_array_and_component(self, itr, name):
        """
        TODO: rename, this does NOT yet return the component.
        Stops at the species + record level.
        the function ... does.

        Returns: tuple of strings
          (species_name, record_name)
        """
        for species_name, species in itr.particles.items():
            for record_name, record in species.items():
                if name == species_name + "_" + record_name:
                    return (species_name, record_name)
        return (None, None)

    def _load_array(self, var, chunk_offset, chunk_extent):
        arrays = []
        for name, scalar in var.items():
            shp = scalar.shape
            comp = scalar.load_chunk(chunk_offset, chunk_extent)
            self._series.flush()
            if not math.isclose(1.0, scalar.unit_SI):
                comp = comp * scalar.unit_SI
            if len(shp) == 3:
                arrays.append(np.transpose(comp, _3d) if mesh_needs_transpose(var) else comp)
            elif len(shp) == 2:
                arrays.append(np.transpose(comp.reshape(shp[0], shp[1], 1), _2d) if mesh_needs_transpose(var) else comp)
            else:
                arrays.append(comp)

        ncomp = len(var)
        if ncomp > 1:
            flt = np.ravel(arrays, order="F")
            return flt.reshape((flt.shape[0] // ncomp, ncomp))
        else:
            return arrays[0].flatten(order="F")

    def _find_array(self, itr, name):
        var = itr.meshes[name]
        theta_modes = None
        if var.geometry == io.Geometry.thetaMode:
            theta_modes = 3  # hardcoded, parse from geometry_parameters
        return (
            var,
            np.array(var.grid_spacing) * var.grid_unit_SI,
            np.array(var.grid_global_offset) * var.grid_unit_SI,
            theta_modes,
        )

    def _get_num_particles(self, itr, species):
        sp = itr.particles[species]
        var = sp["position"]
        return var["x"].shape[0]

    def _load_particle_array(self, itr, species, name, start, end):
        sp = itr.particles[species]
        var = sp[name]
        arrays = []
        for name, scalar in var.items():
            comp = scalar.load_chunk([start], [end - start + 1])
            self._series.flush()
            if not math.isclose(1.0, scalar.unit_SI):
                comp = comp * scalar.unit_SI
            arrays.append(comp)

        ncomp = len(var)
        if ncomp > 1:
            flt = np.ravel(arrays, order="F")
            return flt.reshape((flt.shape[0] // ncomp, ncomp))
        else:
            return arrays[0]

    def _load_particles(self, itr, species, start, end):
        sp = itr.particles[species]
        var = sp["position"]
        ovar = sp["positionOffset"]
        position_arrays = []
        for name, scalar in var.items():
            pos = scalar.load_chunk([start], [end - start + 1])
            self._series.flush()
            pos = pos * scalar.unit_SI
            off = ovar[name].load_chunk([start], [end - start + 1])
            self._series.flush()
            off = off * ovar[name].unit_SI
            position_arrays.append(pos + off)

        flt = np.ravel(position_arrays, order="F")
        num_components = len(var)  # 1D, 2D and 3D positions
        flt = flt.reshape((flt.shape[0] // num_components, num_components))
        # 1D and 2D particles: pad additional components with zero
        while flt.shape[1] < 3:
            flt = np.column_stack([flt, np.zeros_like(flt[:, 0])])
        return flt

    def _load_species(self, itr, species, arrays, piece, npieces, ugrid):
        nparticles = self._get_num_particles(itr, species)
        nlocalparticles = nparticles // npieces
        start = nlocalparticles * piece
        end = start + nlocalparticles - 1
        if piece == npieces - 1:
            end = nparticles - 1
        pts = self._load_particles(itr, species, start, end)
        npts = pts.shape[0]
        ugrid.Points = pts
        for array in arrays:
            if array[1] == "position" or array[1] == "positionOffset":
                continue
            ugrid.PointData.append(
                self._load_particle_array(itr, array[0], array[1], start, end), array[1]
            )
        from vtkmodules.vtkCommonDataModel import vtkCellArray

        ca = vtkCellArray()
        if npts < np.iinfo(np.int32).max:
            dtype = np.int32
        else:
            dtype = np.int64
        offsets = np.linspace(0, npts, npts + 1, dtype=dtype)
        cells = np.linspace(0, npts - 1, npts, dtype=dtype)
        from vtkmodules.numpy_interface import dataset_adapter

        offsets = dataset_adapter.numpyTovtkDataArray(offsets)
        offsets2 = offsets.NewInstance()
        offsets2.DeepCopy(offsets)
        cells = dataset_adapter.numpyTovtkDataArray(cells)
        cells2 = cells.NewInstance()
        cells2.DeepCopy(cells)
        ca.SetData(offsets2, cells2)
        from vtkmodules.util import vtkConstants

        ugrid.VTKObject.SetCells(vtkConstants.VTK_VERTEX, ca)

    def _RequestFieldData(self, executive, output, outInfo, timeInfo):
        from vtkmodules.numpy_interface import dataset_adapter as dsa
        from vtkmodules.vtkCommonDataModel import vtkImageData
        from vtkmodules.vtkCommonExecutionModel import vtkExtentTranslator

        piece = outInfo.Get(executive.UPDATE_PIECE_NUMBER())
        npieces = outInfo.Get(executive.UPDATE_NUMBER_OF_PIECES())
        nghosts = outInfo.Get(executive.UPDATE_NUMBER_OF_GHOST_LEVELS())
        et = vtkExtentTranslator()

        data_time = self._get_update_time(timeInfo)
        idx = self._timemap[data_time]
        itr = self._series.iterations[idx]

        arrays = []
        narrays = self._arrayselection.GetNumberOfArrays()
        for i in range(narrays):
            if self._arrayselection.GetArraySetting(i):
                name = self._arrayselection.GetArrayName(i)
                if not "_lvl" in name:
                    arrays.append((name, self._find_array(itr, name)))
        shp = None
        spacing = None
        theta_modes = None
        grid_offset = None
        for _, ary in arrays:
            var = ary[0]
            for name, scalar in var.items():
                shape = scalar.shape
                break
            spc = list(ary[1])
            if not spacing:
                spacing = spc
            elif spacing != spc:  # all meshes need to have the same spacing
                return 0
            offset = list(ary[2])
            if not grid_offset:
                grid_offset = offset
            elif grid_offset != offset:  # all meshes need to have the same spacing
                return 0
            if not shp:
                shp = shape
            elif shape != shp:  # all arrays needs to have the same shape
                return 0
            if not theta_modes:
                theta_modes = ary[3]

        # fields/meshes: RZ
        if theta_modes and shp is not None:
            et.SetWholeExtent(0, shp[0] - 1, 0, shp[1] - 1, 0, shp[2] - 1)
            et.SetSplitModeToZSlab()  # note: Y and Z are both fine
            et.SetPiece(piece)
            et.SetNumberOfPieces(npieces)
            # et.SetGhostLevel(nghosts)
            et.PieceToExtentByPoints()
            ext = et.GetExtent()

            chunk_offset = [ext[0], ext[2], ext[4]]
            chunk_extent = [
                ext[1] - ext[0] + 1,
                ext[3] - ext[2] + 1,
                ext[5] - ext[4] + 1,
            ]

            data = []
            nthetas = 100  # user parameter
            thetas = np.linspace(0.0, 2.0 * np.pi, nthetas)
            chunk_cyl_shape = (chunk_extent[1], chunk_extent[2], nthetas)  # z, r, theta
            for name, var in arrays:
                cyl_values = np.zeros(chunk_cyl_shape)
                values = self._load_array(var[0], chunk_offset, chunk_extent)
                self._series.flush()

                for ntheta in range(nthetas):
                    cyl_values[:, :, ntheta] += values[0, :, :]
                data.append((name, cyl_values))
                # add all other modes via loop
                # for m in range(theta_modes):

            cyl_spacing = [spacing[0], spacing[1], thetas[1] - thetas[0]]

            z_coord = np.linspace(
                0.0, cyl_spacing[0] * chunk_cyl_shape[0], chunk_cyl_shape[0]
            )
            r_coord = np.linspace(
                0.0, cyl_spacing[1] * chunk_cyl_shape[1], chunk_cyl_shape[1]
            )
            t_coord = thetas

            # to cartesian
            cyl_coords = np.meshgrid(r_coord, z_coord, t_coord)
            rs = cyl_coords[1]
            zs = cyl_coords[0]
            thetas = cyl_coords[2]

            y_coord = rs * np.sin(thetas)
            x_coord = rs * np.cos(thetas)
            z_coord = zs
            # mesh_pts = np.zeros((chunk_cyl_shape[0], chunk_cyl_shape[1], chunk_cyl_shape[2], 3))
            # mesh_pts[:, :, :, 0] = z_coord

            img = vtkImageData()
            img.SetExtent(
                chunk_offset[1],
                chunk_offset[1] + chunk_cyl_shape[0] - 1,
                chunk_offset[2],
                chunk_offset[2] + chunk_cyl_shape[1] - 1,
                0,
                nthetas - 1,
            )
            img.SetSpacing(cyl_spacing)

            imgw = dsa.WrapDataObject(img)
            output.SetPartition(0, img)
            for name, array in data:
                # print(array.shape)
                # print(array.transpose(2,1,0).flatten(order='C').shape)
                imgw.PointData.append(array.transpose(2, 1, 0).flatten(order="C"), name)

            # data = []
            # for name, var in arrays:
            #     unit_SI = var[0].unit_SI
            #     data.append((name, unit_SI * var[0].load_chunk(chunk_offset, chunk_extent)))
            # self._series.flush()

        # fields/meshes: 1D-3D
        elif shp is not None:
            whole_extent = []
            # interleave shape with zeros
            for s in shp:
                whole_extent.append(0)
                whole_extent.append(s - 1)
            # 1D and 2D data: pad with 0, 0 for extra dimensions
            while len(whole_extent) < 6:
                whole_extent.append(0)
                whole_extent.append(0)
            et.SetWholeExtent(*whole_extent)

            et.SetPiece(piece)
            et.SetNumberOfPieces(npieces)
            et.SetGhostLevel(nghosts)
            et.PieceToExtent()
            ext = et.GetExtent()

            chunk_offset = [ext[0], ext[2], ext[4]]
            chunk_extent = [
                ext[1] - ext[0] + 1,
                ext[3] - ext[2] + 1,
                ext[5] - ext[4] + 1,
            ]

            # 1D and 2D data: remove extra dimensions for load
            del chunk_offset[len(shp) :]
            del chunk_extent[len(shp) :]

            data = []
            for name, var in arrays:
                values = self._load_array(var[0], chunk_offset, chunk_extent)
                self._series.flush()
                data.append((name, values))

            # 1D and 2D data: pad spacing with extra 1 and grid_offset with
            # extra 9 values until 3D
            i = iter(spacing)
            spacing = [next(i, 1) for _ in range(3)]
            i = iter(grid_offset)
            grid_offset = [next(i, 0) for _ in range(3)]

            ext = np.array(ext).reshape(3, 2)

            img = vtkImageData()
            if mesh_needs_transpose(var[0]):
                if len(shp) == 3:
                    layout = list(_3d)
                elif len(shp) == 2:
                    layout = list(_2d)
                else:
                    layout = list(_1d)
                ext         = ext[layout].flatten().tolist()
                spacing     = np.array(spacing)[layout].flatten().tolist()
                grid_offset = np.array(grid_offset)[layout].flatten().tolist()
            else:
                ext = ext.flatten().tolist()

            img.SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5])
            img.SetSpacing(spacing)
            img.SetOrigin(grid_offset)

            img.GenerateGhostArray(ext)
            imgw = dsa.WrapDataObject(img)
            output.SetPartition(0, img)
            for name, array in data:
                imgw.PointData.append(array, name)

    def _RequestParticleData(self, executive, poutput, outInfo, timeInfo):
        from vtkmodules.numpy_interface import dataset_adapter as dsa
        from vtkmodules.vtkCommonDataModel import (
            vtkUnstructuredGrid,
            vtkPartitionedDataSet,
        )

        piece = outInfo.Get(executive.UPDATE_PIECE_NUMBER())
        npieces = outInfo.Get(executive.UPDATE_NUMBER_OF_PIECES())

        data_time = self._get_update_time(timeInfo)
        idx = self._timemap[data_time]
        itr = self._series.iterations[idx]

        array_by_species = {}
        narrays = self._particlearrayselection.GetNumberOfArrays()
        for i in range(narrays):
            if self._particlearrayselection.GetArraySetting(i):
                name = self._particlearrayselection.GetArrayName(i)
                names = self._get_particle_array_and_component(itr, name)
                if names[0] and self._speciesselection.ArrayIsEnabled(names[0]):
                    if not names[0] in array_by_species:
                        array_by_species[names[0]] = []
                    array_by_species[names[0]].append(names)
        ids = 0
        for species, arrays in array_by_species.items():
            pds = vtkPartitionedDataSet()
            ugrid = vtkUnstructuredGrid()
            pds.SetPartition(0, ugrid)
            poutput.SetPartitionedDataSet(ids, pds)
            ids += 1
            self._load_species(
                itr, species, arrays, piece, npieces, dsa.WrapDataObject(ugrid)
            )

    def RequestData(self, request, inInfoVec, outInfoVec):
        global _has_openpmd
        if not _has_openpmd:
            print_error("Required Python module 'openpmd_api' missing!")
            return 0

        from vtkmodules.vtkCommonDataModel import (
            vtkDataObject,
            vtkPartitionedDataSet,
            vtkPartitionedDataSetCollection,
        )
        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

        # Which port got an update request: fields (0) or particles (1)
        executive = vtkStreamingDemandDrivenPipeline
        from_port = request.Get(executive.FROM_OUTPUT_PORT())
        numInfo = outInfoVec.GetNumberOfInformationObjects()

        for i in range(numInfo):
            # dictionary-kind object that has all data info requests and output
            # this might only be on port 0 or 1
            outInfo = outInfoVec.GetInformationObject(i)
            # Always get the update time step info from the port
            # that is being updated. Update time may be outdated
            # in the other port - this keeps field & particles in
            # time in sync
            timeInfo = outInfoVec.GetInformationObject(from_port)
            if i == 0:
                output = vtkPartitionedDataSet.GetData(outInfoVec, 0)
                self._RequestFieldData(executive, output, outInfo, timeInfo)
            elif i == 1:
                poutput = vtkPartitionedDataSetCollection.GetData(outInfoVec, 1)
                self._RequestParticleData(executive, poutput, outInfo, timeInfo)
            else:
                print_error(
                    "numInfo number is wrong! It should be exactly 2, is=", numInfo
                )
                return 0

        return 1
