# This file declares a VTK source annotated with
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

@smproxy.source(name="MyPythonSource",
       label="My Python Source")
class MyPythonSource(VTKPythonAlgorithmBase):
    """This creates a Superquadric poly data using a vtkSuperquadricSource internally"""
    def __init__(self):
        super().__init__(nInputPorts=0,
                        nOutputPorts=1,
                        outputType='vtkPolyData')

        from vtkmodules.vtkFiltersSources import vtkSuperquadricSource
        self._realAlgorithm = vtkSuperquadricSource()
        self._realAlgorithm.SetToroidal(1)

    # This is where the output data should be created:
    # you always have to define this method.
    def RequestData(self, request, inInfo, outInfo) -> int:
        from vtkmodules.vtkCommonDataModel import vtkPolyData
        self._realAlgorithm.Update()
        output = vtkPolyData.GetData(outInfo, 0)
        output.ShallowCopy(self._realAlgorithm.GetOutput())
        return 1

    #------------------------------------------------------------------------------
    # In the following, we create some methods to control our source.
    # Those are VTK methods that can be called only from VTK code.
    # Calling Modified() in setters is mandatory to mark the filter
    # as dirty and ensure computation on the next pipeline Update().
    #
    # Using a decorator will create a corresponding Property available
    # from ParaView.
    #------------------------------------------------------------------------------

    # Most features are supported with dedicated decorators
    @smproperty.doublevector(name="Scale", default_values=["1", "1", "1"])
    def SetScale(self, x: int, y: int, z: int):
        self._realAlgorithm.SetScale(x,y,z)
        self.Modified()

    @smproperty.intvector(name="ThetaResolution", number_of_elements="1", default_values="16")
    def SetThetaResolution(self, x: int):
        self._realAlgorithm.SetThetaResolution(x)
        self.Modified()

    # for anything too complex or not yet supported, you can explicitly
    # provide the XML for the method.
    @smproperty.xml("""
        <DoubleVectorProperty name="Center"
            number_of_elements="3"
            default_values="0 0 0"
            command="SetCenter">
            <DoubleRangeDomain name="range" />
            <Documentation>Set center of the superquadric</Documentation>
        </DoubleVectorProperty>""")
    def SetCenter(self, x: float, y: float, z: float):
        self._realAlgorithm.SetCenter(x,y,z)
        self.Modified()

    from typing import Tuple
    # "ValueRangeInfo" and "Value" demonstrate how one can have a slider in the
    # UI for a property with its range fetched at runtime. For int values,
    # use `intvector` and `IntRangeDomain` instead of the double variants used
    # below.
    @smproperty.doublevector(name="ValueRangeInfo", information_only="1")
    def GetValueRange(self) -> Tuple[float, float]:
        return (0, 100)

    @smproperty.doublevector(name="Value", default_values=["0.0"])
    @smdomain.xml(\
        """<DoubleRangeDomain name="range" default_mode="mid">
                <RequiredProperties>
                    <Property name="ValueRangeInfo" function="RangeInfo" />
                </RequiredProperties>
           </DoubleRangeDomain>
        """)
    def SetValue(self, val: float):
        self.value = val
        self.Modified()
