from paraview.util.vtkAlgorithm import *

@smproxy.filter(name="MyPreserveInput")
@smproperty.input(name="Input")
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class PreserveInputTypeFilter(VTKPythonAlgorithmBase):
    """
    Example filter demonstrating how to write a filter that preserves the input
    dataset type.
    """
    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkDataSet")

    def RequestDataObject(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        assert inData is not None
        if outData is None or (not outData.IsA(inData.GetClassName())):
            outData = inData.NewInstance()
            outInfo.GetInformationObject(0).Set(outData.DATA_OBJECT(), outData)
        return super().RequestDataObject(request, inInfo, outInfo)

    def RequestData(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        print("input type =", inData.GetClassName())
        print("output type =", outData.GetClassName())
        assert outData.IsA(inData.GetClassName())
        return 1
