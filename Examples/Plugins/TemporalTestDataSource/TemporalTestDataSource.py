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

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        executive = self.GetExecutive()
        outInfo = outInfoVec.GetInformationObject(0)
        outInfo.Remove(executive.TIME_STEPS())
        outInfo.Remove(executive.TIME_RANGE())

        if self._numOfValues == 0:
            self._numOfValues = 20

        timesteps = range(0,self._numOfValues)
        if timesteps is not None:
            for t in timesteps:
                outInfo.Append(executive.TIME_STEPS(), t)
            outInfo.Append(executive.TIME_RANGE(), timesteps[0])
            outInfo.Append(executive.TIME_RANGE(), timesteps[-1])
        return 1

    def RequestData(self, request, inInfo, outInfo):
        from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
        from vtkmodules.vtkCommonDataModel import vtkImageData
        from vtkmodules.vtkCommonDataModel import vtkFieldData
        from vtkmodules.vtkCommonCore import vtkDoubleArray
        output = vtkMultiBlockDataSet.GetData(outInfo, 0)
        img = vtkImageData()
        img.SetDimensions(16,16,16)
        img.AllocateScalars(11,1)
        output.SetBlock(0, img)

        dataTable = []

        for k in range(0,self._numOfArrays):
            dataTable.append( vtkDoubleArray() )
            dataTable[k].SetNumberOfComponents(1)
            dataTable[k].SetNumberOfTuples(self._numOfValues)
            dataTable[k].SetName("GlobalArray_%d" % k)

            import random
            curvePower = random.randint(1,2)
            scale = random.randint(1,50) / 100.0;
            for i in range(0,self._numOfValues):
                dataTable[k].SetComponent(i, 0, self._valueOffset + scale*pow(i,curvePower))
                #dataTable[1].SetComponent(i, 0, 0.5*i + self._valueOffset)

        field = vtkFieldData()
        for d in dataTable:
            field.AddArray(d)

        img.SetFieldData(field)
        #output.SetFieldData(field)

        return 1

    @smproperty.intvector(name="TimeSteps", default_values=10)
    @smdomain.intrange(min=1, max=50)
    def SetSteps(self, x):
        self._numOfValues = x
        self.Modified()

    @smproperty.intvector(name="ArrayCount", default_values=3)
    @smdomain.intrange(min=1, max=10)
    def SetArrayCount(self, x):
        self._numOfArrays = x
        self.Modified()

    @smproperty.doublevector(name="Value Offset", default_values=0.0)
    def SetOffset(self, x):
        self._valueOffset = x
        self.Modified()
