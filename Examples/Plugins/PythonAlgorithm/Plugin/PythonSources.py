"""This module demonstrates how to implement a source using
VTKPythonAlgorithmBase in ParaView"""


# This is module to import. It provides VTKPythonAlgorithmBase, the base class
# for all python-based vtkAlgorithm subclasses in VTK and decorators used to
# 'register' the algorithm with ParaView along with information about UI.
from paraview.util.vtkAlgorithm import *


#------------------------------------------------------------------------------
# A source example.
#------------------------------------------------------------------------------
@smproxy.source(name="PythonSuperquadricSource",
       label="Python-based Superquadric Source Example")
class PythonSuperquadricSource(VTKPythonAlgorithmBase):
    """This is dummy VTKPythonAlgorithmBase subclass that
    simply puts out a Superquadric poly data using a vtkSuperquadricSource
    internally"""
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self,
                nInputPorts=0,
                nOutputPorts=1,
                outputType='vtkPolyData')
        from vtkmodules.vtkFiltersSources import vtkSuperquadricSource
        self._realAlgorithm = vtkSuperquadricSource()

    def RequestData(self, request, inInfo, outInfo):
        from vtkmodules.vtkCommonDataModel import vtkPolyData
        self._realAlgorithm.Update()
        output = vtkPolyData.GetData(outInfo, 0)
        output.ShallowCopy(self._realAlgorithm.GetOutput())
        return 1

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
    def SetCenter(self, x, y, z):
        self._realAlgorithm.SetCenter(x,y,z)
        self.Modified()

    # In most cases, one can simply use available decorators.
    @smproperty.doublevector(name="Scale", default_values=["1", "1", "1"])
    @smdomain.doublerange()
    def SetScale(self, x, y, z):
        self._realAlgorithm.SetScale(x,y,z)
        self.Modified()

    @smproperty.intvector(name="ThetaResolution", number_of_elements="1", default_values="16")
    def SetThetaResolution(self, x):
        self._realAlgorithm.SetThetaResolution(x)
        self.Modified()

    @smproperty.stringvector(name="ReadOnly", panel_visibility="default", information_only="1", repeatable="1", number_of_elements_per_command="2")
    def GetSomeTable(self):
        return [["x", "y"], ["42", "43"], ["44", "45"]]
    @smproperty.xml("""<PropertyGroup name="Preview" >
    <Property name="ReadOnly" />
    </PropertyGroup>""")
    def Handler(self):
        pass

    @smproperty.intvector(name="PhiResolution", default_values="16")
    @smdomain.intrange(min=0, max=1000)
    def SetPhiResolution(self, x):
        self._realAlgorithm.SetPhiResolution(x)
        self.Modified()

    @smproperty.doublevector(name="Thickness", number_of_elements="1", default_values="0.3333")
    @smdomain.doublerange(min=1e-24, max=1.0)
    def SetThickness(self, x):
        self._realAlgorithm.SetThickness(x)
        self.Modified()

    @smproperty.intvector(name="Toroidal", default_values="0")
    @smdomain.boolean()
    def SetToroidal(self, value):
        self._realAlgorithm.SetToroidal(value)
        self.Modified()

    # "ValueRangeInfo" and "Value" demonstrate how one can have a slider in the
    # UI for a property with its range fetched at runtime. For int values,
    # use `intvector` and `IntRangeDomain` instead of the double variants used
    # below.
    @smproperty.doublevector(name="ValueRangeInfo", information_only="1")
    def GetValueRange(self):
        print("getting range: (0, 100)")
        return (0, 100)

    @smproperty.doublevector(name="Value", default_values=["0.0"])
    @smdomain.xml(\
        """<DoubleRangeDomain name="range" default_mode="mid">
                <RequiredProperties>
                    <Property name="ValueRangeInfo" function="RangeInfo" />
                </RequiredProperties>
           </DoubleRangeDomain>
        """)
    def SetValue(self, val):
        print("settings value:", val)

    # "StringInfo" and "String" demonstrate how one can add a selection widget
    # that lets user choose a string from the list of strings.
    @smproperty.stringvector(name="StringInfo", information_only="1")
    def GetStrings(self):
        return ["one", "two", "three"]

    @smproperty.stringvector(name="String", number_of_elements="1")
    @smdomain.xml(\
        """<StringListDomain name="list">
                <RequiredProperties>
                    <Property name="StringInfo" function="StringInfo"/>
                </RequiredProperties>
            </StringListDomain>
        """)
    def SetString(self, value):
        print("Setting ", value)

    # A proxy property with a proxy list domain
    @smproperty.proxy(command="SetProxyProperty", name="ProxyProperty")
    @smdomain.xml(\
        """
       <ProxyListDomain name="proxy_list">
          <Group name="incremental_point_locators"/>
       </ProxyListDomain>
        """)
    def SetProxyProperty(self, proxy):
        print("ProxyProperty: ", proxy)


def test_PythonSuperquadricSource():
    src = PythonSuperquadricSource()
    src.Update()

    npts = src.GetOutputDataObject(0).GetNumberOfPoints()
    assert npts > 0

    src.SetThetaResolution(50)
    src.SetPhiResolution(50)
    src.Update()
    assert src.GetOutputDataObject(0).GetNumberOfPoints() > npts
