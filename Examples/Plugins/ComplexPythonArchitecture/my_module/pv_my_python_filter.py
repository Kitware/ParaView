# This file declares a VTK filter annotated with
# servermanager information to create a Proxy object that
# can be used from ParaView client.
#
# This class is a VTK class: its content contains VTK code
# so it can reuse any available VTK class and manipulate the
# data itself.
#
# The decorators create the Proxy/Properties part that will
# be available from ParaView

# Import VTKPythonAlgorithmBase, the base class for all python-based vtkAlgorithm.
# Also provides decorators used to 'register' the filter as Proxy in ParaView ServerManager.
from paraview.util.vtkAlgorithm import *

@smproxy.filter(name="MyPythonFilter",
       label="My Python Filter")
@smproperty.input(name="Input")
@smdomain.datatype(dataTypes=["vtkPolyData"], composite_data_supported=False)
class MyPythonFilter(VTKPythonAlgorithmBase):
    """Given a vtkPolyData, adds an Elevation array and a derived version of
    it (either gradient or cumulative sum, see SetDerivatedMethod)"""
    def __init__(self):
        """Initialize a standard filter: 1 input, 1 output.
        Creates a subclass of vtkPolyData"""
        super().__init__(nInputPorts=1,
                         nOutputPorts=1,
                         outputType="vtkPolyData")

        self._method = 0

    # This is where the output data should be created:
    # you always have to define this method.
    def RequestData(self, request, inInfo, outInfo) -> int:
        """Uses VTK to add an "elevation" array. Get its max
        value from internal filter """
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)

        from vtkmodules.vtkFiltersCore import vtkElevationFilter
        elevation = vtkElevationFilter()
        elevation.SetInputData(inData)
        elevation.SetLowPoint(-1, -1, -1)
        elevation.Update()

        from .vtk_internal_filter import vtkMyInternalNumpyFilter
        extract = vtkMyInternalNumpyFilter()
        extract.SetInputConnection(elevation.GetOutputPort())
        extract.SetMethod("gradient" if self._method == 0 else "cumsum")
        extract.Update()

        outData.ShallowCopy(extract.GetOutput())

        return 1

    @smproperty.intvector(name="DerivatedMethod", default_values=0)
    @smdomain.xml("""
    <EnumerationDomain name="enum">
      <Entry text="Gradient" value="0" />
      <Entry text="Cumulative Sum" value="1" />
    </EnumerationDomain>
    """)
    def SetDerivatedMethod(self, method: int):
        """Set the derivated method index. Expect 0 (gradient) or 1 (cumulative sum)"""
        self._method = method
        self.Modified()
