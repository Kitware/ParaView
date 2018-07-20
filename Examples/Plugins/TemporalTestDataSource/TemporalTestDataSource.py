# This is a python plugin that acts as a test data source
# for testing with global temporal data.
# Particularly made for testing the paraview module "Plot Global Temporal Data".
# The plugin creates a MultiBlockDataSet with a dummy uniform grid (vtkImageData).
# Multiple global data arrays are added based on user provided input parameters:
#   - Number of time steps in the temporal data.
#   - Number of global data arrays.
#   - A value offset that shifts the origin of all generated data arrays.

from paraview.util.vtkAlgorithm import *

@smproxy.source(name="PythonTemporalTestDataSource",
                label="Test Data Source for Plotting Global Variables")
class PythonTemporalTestDataSource(VTKPythonAlgorithmBase):
    """A data source that generates dummy MultiBlock dataset with global temporal data arrays.
    This is developed for testing purposes."""
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
                                        nInputPorts=0,
                                        nOutputPorts=1,
                                        outputType='vtkMultiBlockDataSet')
        self._numOfValues = 20
        self._valueOffset = 0.0
        self._numOfArrays = 2
        self._dataTable = []

    def _get_timesteps(self):
        return range(0,self._numOfValues)

    def _get_update_time(self, outInfo):
        executive = self.GetExecutive()
        timesteps = self._get_timesteps()
        if timesteps is None or len(timesteps) == 0:
            return None
        elif outInfo.Has(executive.UPDATE_TIME_STEP()) and len(timesteps) > 0:
            utime = outInfo.Get(executive.UPDATE_TIME_STEP())
            dtime = timesteps[0]
            for atime in timesteps:
                if atime > utime:
                    return dtime
                else:
                    dtime = atime
            return dtime
        else:
            assert(len(timesteps) > 0)
            return timesteps[0]

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        executive = self.GetExecutive()
        outInfo = outInfoVec.GetInformationObject(0)
        outInfo.Remove(executive.TIME_STEPS())
        outInfo.Remove(executive.TIME_RANGE())

        if self._numOfValues == 0:
            self._numOfValues = 20

        timesteps = self._get_timesteps()
        if timesteps is not None:
            outInfo.Set(executive.TIME_STEPS(), timesteps, len(timesteps))
            outInfo.Set(executive.TIME_RANGE(), [timesteps[0],timesteps[-1]], 2)

        return 1

    def RequestData(self, request, inInfo, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
        from vtkmodules.vtkCommonDataModel import vtkImageData
        from vtkmodules.vtkCommonDataModel import vtkFieldData
        from vtkmodules.vtkCommonCore import vtkDoubleArray
        output = vtkMultiBlockDataSet.GetData(outInfoVec, 0)

        if not self._dataTable:
          timesteps = self._get_timesteps()
          for k in range(0,self._numOfArrays):
              self._dataTable.append( vtkDoubleArray() )
              self._dataTable[k].SetNumberOfComponents(1)
              self._dataTable[k].SetNumberOfTuples(self._numOfValues)
              self._dataTable[k].SetName("GlobalArray_%d" % k)

              import random
              curvePower = random.randint(1,2)
              scale = random.randint(1,50) / 100.0;
              for i in timesteps:
                  self._dataTable[k].SetComponent(i, 0, self._valueOffset + scale*pow(i,curvePower))

        field = vtkFieldData()
        for d in self._dataTable:
            field.AddArray(d)

        img = vtkImageData()
        img.SetDimensions(16,16,16)
        img.AllocateScalars(11,1)
        img.SetFieldData(field)
        output.SetBlock(0, img)

        data_time = self._get_update_time(outInfoVec.GetInformationObject(0))
        if data_time is not None:
            output.GetInformation().Set(output.DATA_TIME_STEP(), data_time)

        return 1

    @smproperty.intvector(name="TimeSteps", default_values=10)
    @smdomain.intrange(min=1, max=50)
    def SetSteps(self, x):
        self._numOfValues = x
        self._dataTable = []
        self.Modified()

    @smproperty.intvector(name="ArrayCount", default_values=3)
    @smdomain.intrange(min=1, max=10)
    def SetArrayCount(self, x):
        self._numOfArrays = x
        self._dataTable = []
        self.Modified()

    @smproperty.doublevector(name="Value Offset", default_values=0.0)
    def SetOffset(self, x):
        self._valueOffset = x
        self._dataTable = []
        self.Modified()

    @smproperty.doublevector(name="TimestepValues", information_only="1", si_class="vtkSITimeStepsProperty")
    def GetTimestepValues(self):
        return self._get_timesteps()
