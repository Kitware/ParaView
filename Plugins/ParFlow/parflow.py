"""This module provides ParaView equivalents to several ParFlow
pftools algorithms; these are implemented using VTKPythonAlgorithmBase
subclasses and demonstrate how to access both surface and subsurface
computational grids within a single filter."""

from paraview.util.vtkAlgorithm import *
import numpy as np

class pftools:
    """This class defines some utilities to perform computations similar
    to those implemented by ParFlow's pftools suite."""

    @staticmethod
    def dataPointExtent(dataset):
        """Return the parflow extents in (k,j,i) order of the *point*
        extents of this dataset, which is assumed to be structured.

        Note that VTK normally assumes data is (i,j,k) ordered."""
        ext = tuple(np.flip(dataset.GetDimensions()))
        return ext

    @staticmethod
    def dataCellExtent(dataset):
        """Return the parflow extents in (k,j,i) order of the *cell*
        extents of this dataset, which is assumed to be structured.

        Note that (1) VTK normally uses point counts along axes rather
        than cell counts and (2) assumes data is (i,j,k) ordered."""
        # Subtracting 1 from GetDimensions accounts for point-counts
        # vs cell-counts, while np.flip(...) switches the axis order:
        ext = tuple(np.flip(dataset.GetDimensions()) - 1)
        return ext

    @staticmethod
    def computeTopSurface(ext, mask):
        # I. Reshape the mask and create zk, a column of k-axis indices:
        mr = np.reshape(mask > 0, ext)
        zk = np.reshape(np.arange(ext[0]), (ext[0],1,1))
        # II. The top surface at each (i,j) is the location
        #      of the highest zk-value that is not masked:
        top = np.argmax(mr * zk, axis=0)
        # III. Mark completely-masked columns of cells with an
        #      invalid "top" index of -1:
        # Columns in mask that had no valid value for mr * zk result
        # in entries of "top" that are 0. But 0 is also a valid k index
        # so we mark those which should be marked out with -1:
        mr = (mr == False) # Invert the mask to catch bad cells
        top = top - np.all(mr, axis=0) # Subtract 1 from masked column entries.
        return top

    @staticmethod
    def computeTopSurfaceIndices(top):
        """Convert the "top" surface matrix into an array of indices that
        only include valid top-surface cells. Any negative entry of "top"
        does not have its index included.

        The result is a matrix; each row corresponds to a top-surface cell
        and each column holds a cell's k-, j-, and i-indices, respectively.
        """
        itop = np.array([(top[i,j], j, i) \
            for i in range(top.shape[0]) \
                for j in range(top.shape[1]) \
                    if top[i,j] >= 0])
        return itop

    @staticmethod
    def computeSurfaceStorage(ext, itop, xx, pressure):
        # I. Compute the dx * dy area term using point coordinates from the mesh:
        xt = xx[itop[:,0], itop[:,1], itop[:,2]]
        dx = xx[itop[:,0], itop[:,1], itop[:,2] + 1] - xt
        dy = xx[itop[:,0], itop[:,1] + 1, itop[:,2]] - xt
        # Now dx and dy are matrices whose rows are vectors.
        # Compute the norms of the vectors.
        #  dx = np.sqrt(np.sum(dx * dx,axis=1))
        #  dy = np.sqrt(np.sum(dy * dy,axis=1))
        # To more closely match parflow:
        dx = dx[:,0]
        dy = dy[:,1]

        pp = np.reshape(pressure, ext)
        pt = pp[itop[:,0], itop[:,1], itop[:,2]]
        sus = pt * (pt > 0.0) * dx * dy
        return sus

    @staticmethod
    def computeSubsurfaceStorage(saturation, pressure, volume, porosity, specific_storage):
        """Compute the subsurface storage for the entire domain."""
        import numpy as np
        sbs = saturation * volume * (porosity + pressure * specific_storage)
        return sbs

    @staticmethod
    def computeGroundwaterStorage(saturation, pressure, volume, porosity, specific_storage):
        """Compute the groundwater storage.
        This is the same calculation as subsurface storage, but only
        sums values over cells with a saturation of 1.0.
        """
        import numpy as np
        sbs = (saturation == 1.0) * volume * (porosity + pressure * specific_storage)
        return sbs

    @staticmethod
    def computeSurfaceRunoff(top, xx, pressure, slope, mannings):
        """Compute surface runoff (water leaving the domain boundary
        or flowing into a masked area)"""
        def addToRunoff(runoff, sro, top, slope, pressure, mannings, delta):
            """Given a truthy 2-d array (idx) of places where runoff occurs,
            compute the runoff and add it to the total (sro).
            """
            # We would like to do this:
            #   sro += \
            #       runoff * np.sqrt(np.abs(slope)) / mannings * \
            #       np.power(ptop, 5.0/3.0) * delta
            # ... but we can't since it might result in NaN values where
            # runoff is False. Instead, only compute the runoff exactly
            # at locations where runoff is true:
            ww = np.where(runoff)
            sro[ww] += \
                np.sqrt(np.abs(slope[ww])) / mannings[ww] * \
                np.power(ptop[ww], 5.0/3.0) * delta[ww]

        # Initialize runoff:
        sro = np.zeros(top.shape)
        # Prepare indices and tempororary variables:
        sz = np.prod(top.shape)
        ext = (int(np.prod(pressure.shape)/sz),) + top.shape
        tk = np.reshape(top, sz)
        tj = np.floor(np.arange(sz)/top.shape[1]).astype(int)
        ti = np.arange(sz) % top.shape[1]
        tt = np.zeros(top.shape) # temporary array
        # Subset of pressure at the top surface:
        ptop = np.reshape(np.reshape(pressure, ext)[tk, tj, ti], top.shape)

        # Determine size of top-surface cells along north-south direction:
        delta = np.reshape((xx[tk, tj + 1, ti] - xx[tk, tj, ti])[:,1], top.shape)

        # Fill the temporary array with data shifted south by 1:
        np.concatenate((top[1:,:], -np.ones((1,top.shape[1]))), axis=0, out=tt)
        # Compute conditions for flow exiting to the north:
        cnorth = (top >= 0) & (tt < 0) & (slope[:,:,1] < 0) & (ptop > 0)
        # Now add values to per-cell runoff sro:
        addToRunoff(cnorth, sro, top, slope[:,:,1], pressure, mannings, delta)

        # Fill the temporary array with data shifted north by 1:
        np.concatenate((-np.ones((1,top.shape[1])), top[0:-1,:]), axis=0, out=tt)
        # Compute conditions for flow exiting to the south:
        csouth = (top >= 0) & (tt < 0) & (slope[:,:,1] > 0) & (ptop > 0)
        # Now add values to per-cell runoff sro:
        addToRunoff(csouth, sro, top, slope[:,:,1], pressure, mannings, delta)

        # Determine size of top-surface cells along east-west direction:
        delta = np.reshape((xx[tk, tj, ti + 1] - xx[tk, tj, ti])[:,0], top.shape)

        # Fill the temporary array with data shifted east by 1:
        np.concatenate((top[:,1:], -np.ones((top.shape[0], 1))), axis=1, out=tt)
        # Compute conditions for flow exiting to the west:
        cwest = (top >= 0) & (tt < 0) & (slope[:,:,0] < 0) & (ptop > 0)
        # Now add values to per-cell runoff sro:
        addToRunoff(cwest, sro, top, slope[:,:,0], pressure, mannings, delta)

        # Fill the temporary array with data shifted north by 1:
        np.concatenate((-np.ones((top.shape[0], 1)), top[:,0:-1]), axis=1, out=tt)
        # Compute conditions for flow exiting to the south:
        ceast = (top >= 0) & (tt < 0) & (slope[:,:,0] > 0) & (ptop > 0)
        # Now add values to per-cell runoff sro:
        addToRunoff(ceast, sro, top, slope[:,:,0], pressure, mannings, delta)

        return sro

    @staticmethod
    def computeWaterTableDepth(top, saturation, xx):
        """Compute the depth to the water table.

        The returned array is the distance along the z axis between
        the water surface and ground level. It should always be
        negative (i.e., it is a displacement on the z axis starting
        at the surface).
        """
        sz = np.prod(top.shape)
        ext = (int(np.prod(saturation.shape)/sz),) + top.shape
        sr = np.reshape(saturation >= 1.0, ext)
        zk = np.reshape(np.arange(ext[0]), (ext[0],1,1))
        # The top surface at each (i,j) is the location
        # of the highest zk-value that is not masked:
        depthIdx = np.argmax(sr * zk, axis=0)
        tk = np.reshape(top, sz)
        dk = np.reshape(depthIdx, sz)
        dj = np.floor(np.arange(sz)/top.shape[1]).astype(int)
        di = np.arange(sz) % top.shape[1]
        depth = xx[dk, dj, di, 2] - xx[tk, dj, di, 2]

        # Mark locations where the domain is masked or
        # no water table appears as NaN (not-a-number):
        sr = (sr == False) # Invert the saturation mask
        ww = np.where(np.reshape(np.all(sr, axis=0), sz))
        depth[ww] = np.nan
        return depth

class ParFlowAlgorithm(VTKPythonAlgorithmBase):
    """A base class for algorithms that operate on
    ParFlow simulation data.

    This class provides method implementations that
    simplify working with data from the vtkParFlowMetaReader.
    Specifically,

    1. RequestUpdateExtent will ensure valid extents
    are passed to each of the filter's inputs based on
    their available whole extents. Without this, passing
    both the surface and subsurface meshes to a python
    filter would cause warnings as the default
    implementation simply mirrors the downstream request
    to each of its upstream filters.

    2. RequestDataObject will create the same type of
    output as the input data objects so that either
    image data or structured grids (both of which may
    be output by the reader) can be passed through the
    filter.
    """
    def FillInputPortInformation(self, port, info):
        info.Set(self.INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet")
        return 1

    def RequestDataObject(self, request, inInfoVec, outInfoVec):
        """Create a data object of the same type as the input."""
        from vtkmodules.vtkCommonDataModel import vtkTable, vtkDataSet, vtkPolyData
        from vtkmodules import vtkCommonDataModel as dm
        input0 = vtkDataSet.GetData(inInfoVec[0], 0)
        opt = dm.vtkDataObject.GetData(outInfoVec)

        if opt and opt.IsA(input0.GetClassName()):
            return 1

        opt = input0.NewInstance()
        outInfoVec.GetInformationObject(0).Set(dm.vtkDataObject.DATA_OBJECT(), opt)
        return 1

    def RequestUpdateExtent(self, request, inInfoVec, outInfoVec):
        """Override the default algorithm for updating extents to handle the
        surface and subsurface models, which have different dimensionality.

        Take the requested (downstream) extent and intersect it with the
        available (upstream) extents; use that as the request rather than
        blindly copying the request upstream.
        """
        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline as sddp

        # Determine the port requesting an update from us:
        reqPort = request.Get(sddp.FROM_OUTPUT_PORT())
        # Get that port's information and the extents it is requesting:
        outInfo = self.GetOutputInformation(reqPort)
        esrc = outInfo.Get(sddp.UPDATE_EXTENT())

        # Slice the extents into tuples holding the lo and hi indices:
        losrc = esrc[0:6:2]
        hisrc = esrc[1:6:2]
        np = self.GetNumberOfInputPorts()
        for port in range(np):
            # Loop over all the input connections and set their update extents:
            nc = self.GetNumberOfInputConnections(port)
            for connection in range(nc):
                inInfo = self.GetInputInformation(port, connection)
                # Set UPDATE_EXTENT to be the intersection of the input
                # port's WHOLE_EXTENT with the output port's UPDATE_EXTENT.
                #
                # NB/FIXME: Really this is incorrect. If reqPort is 1, then
                #   someone is asking for an update of the 2-d surface grid
                #   and they could conceivably need the entire 3-d subsurface
                #   grid that overlaps the surface grid in x and y...
                etgt = inInfo.Get(sddp.WHOLE_EXTENT())
                lotgt = etgt[0:6:2]
                hitgt = etgt[1:6:2]
                lodst = [int(max(x)) for x in zip(losrc, lotgt)]
                hidst = [int(min(x)) for x in zip(hisrc, hitgt)]
                edst = [val for tup in zip(lodst, hidst) for val in tup]
                inInfo.Set(sddp.UPDATE_EXTENT(), tuple(edst), 6)
        return 1

@smproxy.filter(label='Subsurface Storage')
@smhint.xml("""
    <ShowInMenu category="ParFlow"/>
    <RepresentationType port="0" view="RenderView" type="Surface" />
""")
@smproperty.input(name="Subsurface", port_index=0)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class SubsurfaceStorage(ParFlowAlgorithm):
    """This algorithm computes the subsurface storage across the entire domain.
    The result is added as field data and marked as time-varying so it
    can be plotted as a timeseries.
    """
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=1, nOutputPorts=1, outputType="vtkDataSet")

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable, vtkDataSet, vtkPolyData
        from vtkmodules.vtkIOExodus import vtkExodusIIReader as e2r
        from vtkmodules.vtkFiltersVerdict import vtkMeshQuality as mq
        from vtkmodules.util.numpy_support import vtk_to_numpy as vton
        from vtkmodules.util.numpy_support import numpy_to_vtk as ntov
        import numpy as np

        ## Fetch the filter inputs and outputs:
        input0 = vtkDataSet.GetData(inInfoVec[0], 0)
        output = vtkDataSet.GetData(outInfoVec, 0)
        output.ShallowCopy(input0)

        ## Fetch input arrays
        cd = output.GetCellData()
        vmask = cd.GetArray('mask')
        vsaturation = cd.GetArray('saturation')
        vporosity = cd.GetArray('porosity')
        vpressure = cd.GetArray('pressure')
        vspecstor = cd.GetArray('specific storage')
        if vmask == None or vsaturation == None or \
           vporosity == None or vpressure == None or \
           vspecstor == None:
            print('Error: A required array was not present.')
            return 0

        ## Compute the volume of each cell:
        mqf = mq()
        mqf.SetHexQualityMeasureToVolume()
        mqf.SetInputDataObject(0, output)
        mqf.Update()
        volume = vton(mqf.GetOutputDataObject(0).GetCellData().GetArray('Quality'))

        ## Get NumPy versions of each array so we can do arithmetic:
        mask = vton(vmask)
        saturation = vton(vsaturation)
        porosity = vton(vporosity)
        pressure = vton(vpressure)
        specstor = vton(vspecstor)

        ## Compute the subsurface storage as sbs
        ## and store it as field data.
        sbs = np.sum(pftools.computeSubsurfaceStorage( \
                saturation, pressure, volume, porosity, specstor))
        arr = ntov(sbs)
        arr.SetName('subsurface storage')
        arr.GetInformation().Set(e2r.GLOBAL_TEMPORAL_VARIABLE(), 1)
        output.GetFieldData().AddArray(arr)

        return 1


@smproxy.filter(label='Water Table Depth')
@smhint.xml("""
    <ShowInMenu category="ParFlow"/>
    <RepresentationType port="0" view="RenderView" type="Surface" />
""")
@smproperty.input(name="Surface", port_index=1)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
@smproperty.input(name="Subsurface", port_index=0)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class WaterTableDepth(ParFlowAlgorithm):
    """This algorithm computes the depth to the water table the domain surface.
    """
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=2, nOutputPorts=1, outputType="vtkDataSet")

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        """Tell ParaView our output extents match the surface, not the subsurface."""
        from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline as sddp

        iin = inInfoVec[1].GetInformationObject(0)
        outInfoVec.GetInformationObject(0).Set(sddp.WHOLE_EXTENT(), iin.Get(sddp.WHOLE_EXTENT()), 6);
        return 1

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable, vtkDataSet, vtkPolyData
        from vtkmodules.vtkIOExodus import vtkExodusIIReader as e2r
        from vtkmodules.vtkFiltersVerdict import vtkMeshQuality as mq
        from vtkmodules.util.numpy_support import vtk_to_numpy as vton
        from vtkmodules.util.numpy_support import numpy_to_vtk as ntov
        import numpy as np

        ## Fetch the filter inputs and outputs:
        input0 = vtkDataSet.GetData(inInfoVec[0], 0)
        input1 = vtkDataSet.GetData(inInfoVec[1], 0)
        output = vtkDataSet.GetData(outInfoVec, 0)
        output.ShallowCopy(input1)

        ## Fetch input arrays
        cd = output.GetCellData()
        scd = input0.GetCellData()
        vmask = scd.GetArray('mask')
        vsaturation = scd.GetArray('saturation')
        if vmask == None or vsaturation == None:
            print('Error: A required array was not present.')
            return 0

        ## Get NumPy versions of each array so we can do arithmetic:
        mask = vton(vmask)
        saturation = vton(vsaturation)

        ## Find the top surface
        ext = pftools.dataCellExtent(input0)
        top = pftools.computeTopSurface(ext, mask)

        ## Compute the water table depth storage as wtd
        ## and store it as cell data.
        ps = pftools.dataPointExtent(input0) + (3,)
        xx = np.reshape(vton(input0.GetPoints().GetData()), ps)
        wtd = pftools.computeWaterTableDepth(top, saturation, xx)
        arr = ntov(wtd)
        arr.SetName('water table depth')
        output.GetCellData().AddArray(arr)

        return 1


@smproxy.filter(label='Water Balance')
@smhint.xml("""
    <ShowInMenu category="ParFlow"/>
    <RepresentationType port="0" view="RenderView" type="Surface" />
""")
@smproperty.input(name="Surface", port_index=1)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
@smproperty.input(name="Subsurface", port_index=0)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class WaterBalance(ParFlowAlgorithm):
    """This algorithm computes the subsurface storage across the entire domain.
    The result is added as field data and marked as time-varying so it
    can be plotted as a timeseries.
    """
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=2, nOutputPorts=1, outputType="vtkDataSet")

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable, vtkDataSet, vtkPolyData
        from vtkmodules.vtkIOExodus import vtkExodusIIReader as e2r
        from vtkmodules.vtkFiltersVerdict import vtkMeshQuality as mq
        from vtkmodules.util.numpy_support import vtk_to_numpy as vton
        from vtkmodules.util.numpy_support import numpy_to_vtk as ntov
        import numpy as np

        ## Fetch the filter inputs and outputs:
        input0 = vtkDataSet.GetData(inInfoVec[0], 0)
        input1 = vtkDataSet.GetData(inInfoVec[1], 0)
        output = vtkDataSet.GetData(outInfoVec, 0)
        output.ShallowCopy(input0)

        ## Fetch input arrays
        cd = output.GetCellData()
        scd = input1.GetCellData()
        vmask = cd.GetArray('mask')
        vsaturation = cd.GetArray('saturation')
        vporosity = cd.GetArray('porosity')
        vpressure = cd.GetArray('pressure')
        vspecstor = cd.GetArray('specific storage')
        vslope = scd.GetArray('slope')
        vmannings = scd.GetArray('mannings')
        if vmask == None or vsaturation == None or \
           vporosity == None or vpressure == None or \
           vspecstor == None or vslope == None or \
           vmannings == None:
            print('Error: A required array was not present.')
            return 0

        ## Compute the volume of each cell:
        mqf = mq()
        mqf.SetHexQualityMeasureToVolume()
        mqf.SetInputDataObject(0, output)
        mqf.Update()
        volume = vton(mqf.GetOutputDataObject(0).GetCellData().GetArray('Quality'))

        ## Get NumPy versions of each array so we can do arithmetic:
        mask = vton(vmask)
        saturation = vton(vsaturation)
        porosity = vton(vporosity)
        pressure = vton(vpressure)
        specstor = vton(vspecstor)
        slope = vton(vslope)
        mannings = vton(vmannings)

        ## Compute the subsurface storage as sbs
        ## and store it as field data.
        sbs = np.sum(pftools.computeSubsurfaceStorage( \
                saturation, pressure, volume, porosity, specstor))
        arr = ntov(sbs)
        arr.SetName('subsurface storage')
        arr.GetInformation().Set(e2r.GLOBAL_TEMPORAL_VARIABLE(), 1)
        output.GetFieldData().AddArray(arr)

        ## Find the top surface
        ext = pftools.dataCellExtent(input0)
        top = pftools.computeTopSurface(ext, mask)
        itop = pftools.computeTopSurfaceIndices(top)

        ## Compute the surface storage
        ps = pftools.dataPointExtent(input0) + (3,)
        xx = np.reshape(vton(output.GetPoints().GetData()), ps)
        sus = np.sum(pftools.computeSurfaceStorage(ext, itop, xx, pressure))
        arr = ntov(sus)
        arr.SetName('surface storage')
        arr.GetInformation().Set(e2r.GLOBAL_TEMPORAL_VARIABLE(), 1)
        output.GetFieldData().AddArray(arr)

        ## Compute the surface runoff
        slope = np.reshape(slope, top.shape + (2,))
        mannings = np.reshape(mannings, top.shape)
        sro = np.sum(pftools.computeSurfaceRunoff(top, xx, pressure, slope, mannings))
        arr = ntov(sro)
        arr.SetName('surface runoff')
        arr.GetInformation().Set(e2r.GLOBAL_TEMPORAL_VARIABLE(), 1)
        output.GetFieldData().AddArray(arr)

        return 1

if __name__ == "__main__":
    from paraview.detail.pythonalgorithm import get_plugin_xmls
    from xml.dom.minidom import parseString
    for xml in get_plugin_xmls(globals()):
        dom = parseString(xml)
        print(dom.toprettyxml(" ","\n"))
