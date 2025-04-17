import os.path
from paraview.util.vtkAlgorithm import *
from vtkmodules.util.xarray_support import vtkXArrayCFReader
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

@smproxy.reader(name="XArrayCFReader", label="XArrayCF Reader",
                extensions="nc h5 zgroup grib", file_description="XArray files")
class XArrayCFReader(vtkXArrayCFReader):
    '''Reads data from a file using the XArray readers and then connects
    the XArray data to the vtkNetCDFCFREader (using zero-copy when
    possible). At the moment, data is copied for coordinates (because
    they are converted to double in the reader) and for certain data
    that is subset either in XArray or in VTK.  Lazy loading in XArray
    is respected, that is data is accessed only when it is needed.
    Time is passed to VTK either as an int64 for datetime64 or
    timedelta64, or as a double (using cftime.toordinal) for cftime.
    '''
    def __init__(self):
        super().__init__()

    @smproperty.stringvector(name="FileName")
    @smdomain.filelist()
    @smhint.filechooser(extensions="nc,h5,zgroup,grib", file_description="XArray files")
    def SetFileName(self, name):
        """Specify filename for the file to read."""
        if os.path.basename(name) == '.zgroup':
            super().SetFileName(os.path.dirname(name))
        else:
            super().SetFileName(name)


    @smproperty.stringvector(name="DimensionInfo", information_only="1")
    @smdomain.xml("""<StringArrayHelper />""")
    def GetAllDimensions(self):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        all_dims = self._reader.GetAllDimensions()
        return all_dims


    @smproperty.stringvector(name="Dimensions", number_of_elements="1")
    @smdomain.xml(\
        """<StringListDomain name="array_list">
                <RequiredProperties>
                    <Property name="DimensionInfo" function="ArrayList"/>
                </RequiredProperties>
            </StringListDomain>
        <Documentation>Load the grid with the given dimensions. Any arrays that
        conform to these dimensions will be loaded.</Documentation>
        """)
    def SetDimensions(self, dims):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        if dims != 'None':
            self._reader.SetDimensions(dims)
            self.Modified()


    @smproperty.intvector(name="SphericalCoordinates", number_of_elements="1",
                          default_values="1")
    @smdomain.xml(\
        """<BooleanDomain name="bool" />
        <Documentation>If on, then data with latitude/longitude dimensions will
        be read in as curvilinear data shaped like spherical coordinates. If
        false, then the data will always be read in Cartesian
        coordinates.</Documentation>
        """)
    def SetSphericalCoordinates(self, spherical):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        self._reader.SetSphericalCoordinates(spherical)
        self.Modified()

    @smproperty.doublevector(name="VerticalScale", number_of_elements="1",
                             default_values="1")
    @smdomain.xml(\
        """<DoubleRangeDomain name="range" />
        <Documentation>The scale of the vertical component of spherical
        coordinates. It is common to write the vertical component with respect
        to something other than the center of the sphere (for example, the
        surface). In this case, it might be necessary to scale and/or bias the
        vertical height. The height will become height*scale + bias. Keep in
        mind that if the positive attribute of the vertical dimension is down,
        then the height is negated. The scaling will be adjusted if it results
        in invalid (negative) vertical values.</Documentation>
        """)
    def SetVerticalScale(self, scale):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        self._reader.SetVerticalScale(scale)
        self.Modified()


    @smproperty.doublevector(name="VerticalBias", number_of_elements="1",
                             default_values="0")
    @smdomain.xml(\
        """<DoubleRangeDomain name="range" />
        <Documentation>The bias of the vertical component of spherical
        coordinates. It is common to write the vertical component with respect
        to something other than the center of the sphere (for example, the
        surface). In this case, it might be necessary to scale and/or bias the
        vertical height. The height will become height*scale + bias. Keep in
        mind that if the positive attribute of the vertical dimension is down,
        then the height is negated. The scaling will be adjusted if it results
        in invalid (negative) vertical values.</Documentation>
        """)
    def SetVerticalBias(self, bias):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        self._reader.SetVerticalBias(bias)
        self.Modified()

    @smproperty.intvector(name="ReplaceFillValueWithNan", number_of_elements="1",
                          default_values="0")
    @smdomain.xml(\
        """<BooleanDomain name="bool" />
        <Documentation>If on, any float or double variable read that has a
        _FillValue attribute will have that fill value replaced with a
        not-a-number (NaN) value. The advantage of setting these to NaN values
        is that, if implemented properly by the system and careful math
        operations are used, they can implicitly be ignored by calculations
        like finding the range of the values. That said, this option should be
        used with caution as VTK does not fully support NaN values and
        therefore odd calculations may occur.</Documentation>
        """)
    def SetReplaceFillValueWithNan(self, replace):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        self._reader.SetReplaceFillValueWithNan(replace)
        self.Modified()

    @smproperty.intvector(name="OutputType", number_of_elements="1",
                          default_values="-1")
    @smdomain.xml(\
        """<EnumerationDomain name="enum">
          <Entry text="Automatic"
                 value="-1" />
          <Entry text="Image"
                 value="6" />
          <Entry text="Rectilinear"
                 value="3" />
          <Entry text="Structured"
                 value="2" />
          <Entry text="Unstructured"
                 value="4" />
        </EnumerationDomain>
        <Documentation>Specifies the type of data that the reader creates. If
        Automatic, the reader will use the most appropriate grid type for the
        data read. Note that not all grid types support all data. A warning is
        issued if a mismatch occurs.</Documentation>
        """)
    def SetOutputType(self, output_type):
        # cannot use super(). see https://stackoverflow.com/questions/12047847/super-object-not-calling-getattr
        self._reader.SetOutputType(output_type)
        self.Modified()


    @smproperty.doublevector(name="TimestepValues", information_only="1", si_class="vtkSITimeStepsProperty")
    def GetTimestepValues(self):
        information = self.GetOutputInformation(0)
        if information.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()):
            times = information.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
            return times
        else:
            return []
