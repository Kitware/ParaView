from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from .. import print_error

try:
    import numpy as np
    import openpmd_api as io
    _has_openpmd = True
except ImportError as ie:
    print_error(
        "Missing required Python modules/packages. Algorithms in this module may "
        "not work as expected! \n {0}".format(ie))
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

class openPMDReader(VTKPythonAlgorithmBase):
    """A reader that reads openPMD format."""
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0, nOutputPorts=2, outputType='vtkPartitionedDataSet')
        self._filename = None
        self._timevalues = None
        self._series = None
        self._timemap = {}

        from vtkmodules.vtkCommonCore import vtkDataArraySelection
        self._arrayselection = vtkDataArraySelection()
        self._arrayselection.AddObserver("ModifiedEvent", createModifiedCallback(self))

        self._speciesselection = vtkDataArraySelection()
        self._speciesselection.AddObserver("ModifiedEvent", createModifiedCallback(self))

        self._particlearrayselection = vtkDataArraySelection()
        self._particlearrayselection.AddObserver("ModifiedEvent", createModifiedCallback(self))

    def _get_update_time(self, outInfo):
        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
        executive = vtkStreamingDemandDrivenPipeline
        timevalues = self._timevalues
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
            assert(len(timevalues) > 0)
            return timevalues[0]

    def _get_array_selection(self):
        return self._arrayselection

    def _get_particle_array_selection(self):
        return self._particlearrayselection

    def _get_species_selection(self):
        return self._speciesselection

    def SetFileName(self, name):
        """Specify filename for the file to read."""
        if self._filename != name:
            self._filename = name
            self._timevalues = None
            if self._series:
                self._series = None
            self.Modified()

    def GetTimestepValues(self):
        return self._timevalues()

    def GetDataArraySelection(self):
        return self._get_array_selection()

    def GetSpeciesSelection(self):
        return self._get_species_selection()

    def GetParticleArraySelection(self):
        return self._get_particle_array_selection()

    def FillOutputPortInformation(self, port, info):
        from vtkmodules.vtkCommonDataModel import vtkDataObject
        if port == 0:
            info.Set(vtkDataObject.DATA_TYPE_NAME(), "vtkPartitionedDataSet")
        else:
            info.Set(vtkDataObject.DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection")
        return 1

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        global _has_openpmd
        if not _has_openpmd:
            print_error("Required Python module 'openpmd_api' missing!")
            return 0

        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline, vtkAlgorithm
        executive = vtkStreamingDemandDrivenPipeline
        for i in (0,1):
            outInfo = outInfoVec.GetInformationObject(i)
            outInfo.Remove(executive.TIME_STEPS())
            outInfo.Remove(executive.TIME_RANGE())
            outInfo.Set(vtkAlgorithm.CAN_HANDLE_PIECE_REQUEST(), 1)

        # Why is this a string when it is None?
        if self._filename == 'None':
            return 1

        mfile = open(self._filename, "r")
        pattern = mfile.readlines()[0][0:-1]
        del mfile


        import os
        if not self._series:
            self._series = io.Series(os.path.dirname(self._filename)+'/'+pattern, io.Access_Type.read_only)
        # This is how we get time values and arrays
        self._timemap = {}
        timevalues = []
        arrays = set()
        particles = set()
        species = set()
        for idx, iteration in self._series.iterations.items():
            time = iteration.time()
            timevalues.append(time)
            self._timemap[time] = idx
            arrays.update([
                mesh_name
                for mesh_name, mesh in iteration.meshes.items()])
            particles.update([
                species_name + "_" + record_name
                for species_name, species in iteration.particles.items()
                for record_name, record in species.items()
                ])
            species.update([
                species_name
                for species_name, _ in iteration.particles.items()
                ])

        for array in arrays:
            self._arrayselection.AddArray(array)
        for particle_array in particles:
            self._particlearrayselection.AddArray(particle_array)
        for species_name in species:
            self._speciesselection.AddArray(species_name)

        timesteps = list(self._series.iterations)
        self._timevalues = timevalues
        if len(timevalues) > 0:
            for i in (0,1):
                outInfo = outInfoVec.GetInformationObject(i)
                for t in timevalues:
                    outInfo.Append(executive.TIME_STEPS(), t)
                outInfo.Append(executive.TIME_RANGE(), timevalues[0])
                outInfo.Append(executive.TIME_RANGE(), timevalues[-1])
        return 1

    def _get_array_and_component(self, itr, name):
        for mesh_name, mesh in itr.meshes.items():
            if mesh_name == name:
                return (mesh_name, None)
            for comp_name, _ in mesh.items():
                if name == mesh_name + "_" + comp_name:
                    return (mesh_name, comp_name)
        return (None, None)

    def _get_particle_array_and_component(self, itr, name):
        for species_name, species in itr.particles.items():
            for record_name, record in species.items():
                if name == species_name + "_" + record_name:
                    return (species_name, record_name)
        return (None, None)

    def _load_array(self, var, chunk_offset, chunk_extent):
        arrays = []
        for name, scalar in var.items():
            comp = scalar.load_chunk(chunk_offset, chunk_extent)
            self._series.flush()
            comp = comp * scalar.unit_SI
            arrays.append(comp)

        ncomp = len(var)
        if ncomp > 1:
            flt = np.ravel(arrays, order='F')
            return flt.reshape((flt.shape[0]//ncomp, ncomp))
        else:
            return arrays[0].flatten(order='F')

    def _find_array(self, itr, name):
        var = itr.meshes[name]
        theta_modes = None
        if var.geometry == io.Geometry.thetaMode:
            theta_modes = 3 # hardcoded, parse from geometry_parameters
        return (var, np.array(var.grid_spacing)*var.grid_unit_SI, np.array(var.grid_global_offset)*var.grid_unit_SI, theta_modes)

    def _get_num_particles(self, itr, species):
        sp = itr.particles[species]
        var = sp["position"]
        return var['x'].shape[0]

    def _load_particle_array(self, itr, species, name, start, end):
        sp = itr.particles[species]
        var = sp[name]
        arrays = []
        for name, scalar in var.items():
            comp = scalar.load_chunk([start], [end-start+1])
            self._series.flush()
            comp = comp * scalar.unit_SI
            arrays.append(comp)

        ncomp = len(var)
        if ncomp > 1:
            flt = np.ravel(arrays, order='F')
            return flt.reshape((flt.shape[0]//ncomp, ncomp))
        else:
            return arrays[0]

    def _load_particles(self, itr, species, start, end):
        sp = itr.particles[species]
        var = sp["position"]
        ovar = sp["positionOffset"]
        position_arrays = []
        for name, scalar in var.items():
            pos = scalar.load_chunk([start], [end-start+1])
            self._series.flush()
            pos = pos * scalar.unit_SI
            off = ovar[name].load_chunk([start], [end-start+1])
            self._series.flush()
            off = off * ovar[name].unit_SI
            position_arrays.append(pos + off)

        flt = np.ravel(position_arrays, order='F')
        return flt.reshape((flt.shape[0]//3, 3))

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
            if array[1] == 'position' or array[1] == 'positionOffset':
                continue
            ugrid.PointData.append(
                self._load_particle_array(itr, array[0], array[1], start, end),
                array[1])
        from vtkmodules.vtkCommonDataModel import vtkCellArray
        ca = vtkCellArray()
        if npts < np.iinfo(np.int32).max:
            dtype = np.int32
        else:
            dtype = np.int64
        offsets = np.linspace(0, npts, npts+1, dtype=dtype)
        cells = np.linspace(0, npts-1, npts, dtype=dtype)
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


    def RequestData(self, request, inInfoVec, outInfoVec):
        global _has_openpmd
        if not _has_openpmd:
            print_error("Required Python module 'openpmd_api' missing!")
            return 0

        from vtkmodules.vtkCommonDataModel import vtkImageData, vtkUnstructuredGrid
        from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSet,vtkPartitionedDataSetCollection
        from vtkmodules.vtkCommonExecutionModel import vtkExtentTranslator, vtkStreamingDemandDrivenPipeline
        from vtkmodules.numpy_interface import dataset_adapter as dsa

        executive = vtkStreamingDemandDrivenPipeline
        output = vtkPartitionedDataSet.GetData(outInfoVec, 0)
        poutput = vtkPartitionedDataSetCollection.GetData(outInfoVec, 1)
        outInfo = outInfoVec.GetInformationObject(0)
        piece = outInfo.Get(executive.UPDATE_PIECE_NUMBER())
        npieces = outInfo.Get(executive.UPDATE_NUMBER_OF_PIECES())
        nghosts = outInfo.Get(executive.UPDATE_NUMBER_OF_GHOST_LEVELS())
        et = vtkExtentTranslator()

        data_time = self._get_update_time(outInfo)
        idx = self._timemap[data_time]

        itr = self._series.iterations[idx]
        arrays = []
        narrays = self._arrayselection.GetNumberOfArrays()
        for i in range(narrays):
            if self._arrayselection.GetArraySetting(i):
                name = self._arrayselection.GetArrayName(i)
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
            elif spacing != spc: # all meshes need to have the same spacing
                return 0
            offset = list(ary[2])
            if not grid_offset:
                grid_offset = offset
            elif grid_offset != offset: # all meshes need to have the same spacing
                return 0
            if not shp:
                shp = shape
            elif shape != shp: # all arrays needs to have the same shape
                return 0
            if not theta_modes:
                theta_modes = ary[3]

        if theta_modes:
            et.SetWholeExtent(0, shp[0]-1, 0, shp[1]-1, 0, shp[2]-1)
            et.SetSplitModeToZSlab() # note: Y and Z are both fine
            et.SetPiece(piece)
            et.SetNumberOfPieces(npieces)
            # et.SetGhostLevel(nghosts)
            et.PieceToExtentByPoints()
            ext = et.GetExtent()

            chunk_offset = [ext[0], ext[2], ext[4]]
            chunk_extent = [ext[1] - ext[0] + 1, ext[3] - ext[2] + 1, ext[5] - ext[4] + 1]

            data = []
            nthetas = 100  # user parameter
            thetas = np.linspace(0., 2.*np.pi, nthetas)
            chunk_cyl_shape = (chunk_extent[1], chunk_extent[2], nthetas)  # z, r, theta
            for name, var in arrays:
                cyl_values = np.zeros(chunk_cyl_shape)
                values = self._load_array(var[0], chunk_offset, chunk_extent)
                self._series.flush()

                print(chunk_cyl_shape)
                print(values.shape)
                print("+++++++++++")
                for ntheta in range(nthetas):
                    cyl_values[:, :, ntheta] += values[0, :, :]
                data.append((name, cyl_values))
                # add all other modes via loop
                # for m in range(theta_modes):

            cyl_spacing = [spacing[0], spacing[1], thetas[1]-thetas[0]]

            z_coord = np.linspace(0., cyl_spacing[0] * chunk_cyl_shape[0], chunk_cyl_shape[0])
            r_coord = np.linspace(0., cyl_spacing[1] * chunk_cyl_shape[1], chunk_cyl_shape[1])
            t_coord = thetas

            # to cartesian
            print(z_coord.shape, r_coord.shape, t_coord.shape)
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
                chunk_offset[1], chunk_offset[1] + chunk_cyl_shape[0] - 1,
                chunk_offset[2], chunk_offset[2] + chunk_cyl_shape[1] - 1,
                0, nthetas-1)
            img.SetSpacing(cyl_spacing)

            imgw = dsa.WrapDataObject(img)
            output.SetPartition(0, img)
            for name, array in data:
                # print(array.shape)
                # print(array.transpose(2,1,0).flatten(order='C').shape)
                imgw.PointData.append(array.transpose(2,1,0).flatten(order='C'), name)


            # data = []
            # for name, var in arrays:
            #     unit_SI = var[0].unit_SI
            #     data.append((name, unit_SI * var[0].load_chunk(chunk_offset, chunk_extent)))
            # self._series.flush()

        else:
            et.SetWholeExtent(0, shp[0]-1, 0, shp[1]-1, 0, shp[2]-1)
            et.SetPiece(piece)
            et.SetNumberOfPieces(npieces)
            et.SetGhostLevel(nghosts)
            et.PieceToExtent()
            ext = et.GetExtent()

            chunk_offset = [ext[0], ext[2], ext[4]]
            chunk_extent = [ext[1] - ext[0] + 1, ext[3] - ext[2] + 1, ext[5] - ext[4] + 1]

            data = []
            for name, var in arrays:
                values = self._load_array(var[0], chunk_offset, chunk_extent)
                self._series.flush()
                data.append((name, values))

            img = vtkImageData()
            img.SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5])
            img.SetSpacing(spacing)
            img.SetOrigin(grid_offset)

            et.SetGhostLevel(0)
            et.PieceToExtent()
            ext = et.GetExtent()
            ext = [ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]]
            img.GenerateGhostArray(ext)
            imgw = dsa.WrapDataObject(img)
            output.SetPartition(0, img)
            for name, array in data:
                imgw.PointData.append(array, name)

        itr = self._series.iterations[idx]
        array_by_species = {}
        narrays = self._particlearrayselection.GetNumberOfArrays()
        for i in range(narrays):
            if self._particlearrayselection.GetArraySetting(i):
                name = self._particlearrayselection.GetArrayName(i)
                names = self._get_particle_array_and_component(
                    itr, name)
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
                itr, species, arrays, piece, npieces, dsa.WrapDataObject(ugrid))

        return 1
