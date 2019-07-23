"""This module demonstrates various ways of adding
VTKPythonAlgorithmBase subclasses as filters, sources, readers,
and writers in ParaView"""


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
    @smproperty.doublevector(name="Scale", default_values=[1, 1, 1])
    @smdomain.doublerange()
    def SetScale(self, x, y, z):
        self._realAlgorithm.SetScale(x,y,z)
        self.Modified()

    @smproperty.intvector(name="ThetaResolution", default_values=16)
    def SetThetaResolution(self, x):
        self._realAlgorithm.SetThetaResolution(x)
        self.Modified()

    @smproperty.intvector(name="PhiResolution", default_values=16)
    @smdomain.intrange(min=0, max=1000)
    def SetPhiResolution(self, x):
        self._realAlgorithm.SetPhiResolution(x)
        self.Modified()

    @smproperty.doublevector(name="Thickness", default_values=0.3333)
    @smdomain.doublerange(min=1e-24, max=1.0)
    def SetThickness(self, x):
        self._realAlgorithm.SetThickness(x)
        self.Modified()

    # "ValueRangeInfo" and "Value" demonstrate how one can have a slider in the
    # UI for a property with its range fetched at runtime. For int values,
    # use `intvector` and `IntRangeDomain` instead of the double variants used
    # below.
    @smproperty.doublevector(name="ValueRangeInfo", information_only="1")
    def GetValueRange(self):
        print("getting range: (0, 100)")
        return (0, 100)

    @smproperty.doublevector(name="Value", default_values=[0.0])
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




#------------------------------------------------------------------------------
# A reader example.
#------------------------------------------------------------------------------
def createModifiedCallback(anobject):
    import weakref
    weakref_obj = weakref.ref(anobject)
    anobject = None
    def _markmodified(*args, **kwars):
        o = weakref_obj()
        if o is not None:
            o.Modified()
    return _markmodified

# To add a reader, we can use the following decorators
#   @smproxy.source(name="PythonCSVReader", label="Python-based CSV Reader")
#   @smhint.xml("""<ReaderFactory extensions="csv" file_description="Numpy CSV files" />""")
# or directly use the "@reader" decorator.
@smproxy.reader(name="PythonCSVReader", label="Python-based CSV Reader",
                extensions="csv", file_description="CSV files")
class PythonCSVReader(VTKPythonAlgorithmBase):
    """A reader that reads a CSV file. If the CSV has a "time" column, then
    the data is treated as a temporal dataset"""
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0, nOutputPorts=1, outputType='vtkTable')
        self._filename = None
        self._ndata = None
        self._timesteps = None

        from vtkmodules.vtkCommonCore import vtkDataArraySelection
        self._arrayselection = vtkDataArraySelection()
        self._arrayselection.AddObserver("ModifiedEvent", createModifiedCallback(self))

    def _get_raw_data(self, requested_time=None):
        if self._ndata is not None:
            if requested_time is not None:
                return self._ndata[self._ndata["time"]==requested_time]
            return self._ndata

        if self._filename is None:
            # Note, exceptions are totally fine!
            raise RuntimeError("No filename specified")

        import numpy
        self._ndata = numpy.genfromtxt(self._filename, dtype=None, names=True, delimiter=',', autostrip=True)
        self._timesteps = None
        if "time" in self._ndata.dtype.names:
            self._timesteps = numpy.sort(numpy.unique(self._ndata["time"]))

        for aname in self._ndata.dtype.names:
            # note, this doesn't change MTime on the array selection, which is
            # good!
            self._arrayselection.AddArray(aname)
        return self._get_raw_data(requested_time)

    def _get_timesteps(self):
        self._get_raw_data()
        return self._timesteps.tolist() if self._timesteps is not None else None

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

    def _get_array_selection(self):
        return self._arrayselection

    @smproperty.stringvector(name="FileName")
    @smdomain.filelist()
    @smhint.filechooser(extensions="csv", file_description="Numpy CSV files")
    def SetFileName(self, name):
        """Specify filename for the file to read."""
        if self._filename != name:
            self._filename = name
            self._ndata = None
            self._timesteps = None
            self.Modified()

    @smproperty.doublevector(name="TimestepValues", information_only="1", si_class="vtkSITimeStepsProperty")
    def GetTimestepValues(self):
        return self._get_timesteps()

    # Array selection API is typical with readers in VTK
    # This is intended to allow ability for users to choose which arrays to
    # load. To expose that in ParaView, simply use the
    # smproperty.dataarrayselection().
    # This method **must** return a `vtkDataArraySelection` instance.
    @smproperty.dataarrayselection(name="Arrays")
    def GetDataArraySelection(self):
        return self._get_array_selection()

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        executive = self.GetExecutive()
        outInfo = outInfoVec.GetInformationObject(0)
        outInfo.Remove(executive.TIME_STEPS())
        outInfo.Remove(executive.TIME_RANGE())

        timesteps = self._get_timesteps()
        if timesteps is not None:
            for t in timesteps:
                outInfo.Append(executive.TIME_STEPS(), t)
            outInfo.Append(executive.TIME_RANGE(), timesteps[0])
            outInfo.Append(executive.TIME_RANGE(), timesteps[-1])
        return 1

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable
        from vtkmodules.numpy_interface import dataset_adapter as dsa

        data_time = self._get_update_time(outInfoVec.GetInformationObject(0))
        raw_data = self._get_raw_data(data_time)
        output = dsa.WrapDataObject(vtkTable.GetData(outInfoVec, 0))
        for name in raw_data.dtype.names:
            if self._arrayselection.ArrayIsEnabled(name):
                output.RowData.append(raw_data[name], name)

        if data_time is not None:
            output.GetInformation().Set(output.DATA_TIME_STEP(), data_time)
        return 1

#------------------------------------------------------------------------------
# A writer example.
#------------------------------------------------------------------------------
@smproxy.writer(extensions="npz", file_description="NumPy Compressed Arrays", support_reload=False)
@smproperty.input(name="Input", port_index=0)
@smdomain.datatype(dataTypes=["vtkTable"], composite_data_supported=False)
class NumpyWriter(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=1, nOutputPorts=0, inputType='vtkTable')
        self._filename = None

    @smproperty.stringvector(name="FileName", panel_visibility="never")
    @smdomain.filelist()
    def SetFileName(self, fname):
        """Specify filename for the file to write."""
        if self._filename != fname:
            self._filename = fname
            self.Modified()

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable
        from vtkmodules.numpy_interface import dataset_adapter as dsa

        table = dsa.WrapDataObject(vtkTable.GetData(inInfoVec[0], 0))
        kwargs = {}
        for aname in table.RowData.keys():
            kwargs[aname] = table.RowData[aname]

        import numpy
        numpy.savez_compressed(self._filename, **kwargs)
        return 1

    def Write(self):
        self.Modified()
        self.Update()

#------------------------------------------------------------------------------
# A filter example.
#------------------------------------------------------------------------------
@smproxy.filter()
@smproperty.input(name="InputTable", port_index=1)
@smdomain.datatype(dataTypes=["vtkTable"], composite_data_supported=False)
@smproperty.input(name="InputDataset", port_index=0)
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class ExampleTwoInputFilter(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=2, nOutputPorts=1, outputType="vtkPolyData")

    def FillInputPortInformation(self, port, info):
        if port == 0:
            info.Set(self.INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet")
        else:
            info.Set(self.INPUT_REQUIRED_DATA_TYPE(), "vtkTable")
        return 1

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable, vtkDataSet, vtkPolyData
        input0 = vtkDataSet.GetData(inInfoVec[0], 0)
        input1 = vtkDataSet.GetData(inInfoVec[1], 0)
        output = vtkPolyData.GetData(outInfoVec, 0)
        # do work
        print("Pretend work done!")
        return 1

def test_PythonSuperquadricSource():
    src = PythonSuperquadricSource()
    src.Update()

    npts = src.GetOutputDataObject(0).GetNumberOfPoints()
    assert npts > 0

    src.SetThetaResolution(50)
    src.SetPhiResolution(50)
    src.Update()
    assert src.GetOutputDataObject(0).GetNumberOfPoints() > npts

def test_PythonCSVReader(fname):
    reader = PythonCSVReader()
    reader.SetFileName(fname)
    reader.Update()
    assert reader.GetOutputDataObject(0).GetNumberOfRows() > 0

if __name__ == "__main__":
    #test_PythonSuperquadricSource()
    #test_PythonCSVReader("/tmp/data.csv")

    from paraview.detail.pythonalgorithm import get_plugin_xmls
    from xml.dom.minidom import parseString
    for xml in get_plugin_xmls(globals()):
        dom = parseString(xml)
        print(dom.toprettyxml(" ","\n"))
