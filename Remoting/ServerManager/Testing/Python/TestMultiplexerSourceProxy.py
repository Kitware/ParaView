from paraview.simple import *
from paraview import smtesting
import os.path

xml = """
<ServerManagerConfiguration>
  <ProxyGroup name="filters">
    <MultiplexerSourceProxy name="Multiplexer">
      <InputProperty name="Input">
        <Documentation>
          Specify the input to this filter.
        </Documentation>
        <MultiplexerInputDomain name="input" />
      </InputProperty>
    </MultiplexerSourceProxy>

    <SourceProxy name="TypeSphere" class="vtkSphereSource">
      <InputProperty name="Input">
        <DataTypeDomain name="input_type">
          <DataType value="vtkPolyData" />
        </DataTypeDomain>
      </InputProperty>
      <DoubleVectorProperty command="SetCenter"
                            default_values="0.0 0.0 0.0"
                            name="Center"
                            number_of_elements="3"
                            panel_visibility="default">
        <DoubleRangeDomain name="range" />
        <Documentation>This property specifies the 3D coordinates for the
        center of the sphere.</Documentation>
      </DoubleVectorProperty>

      <Hints>
        <!-- not adding optional <LinkProperties/> element -->
        <MultiplexerSourceProxy proxygroup="filters" proxyname="Multiplexer" />
      </Hints>
    </SourceProxy>

    <SourceProxy name="TypeWavelet" class="vtkRTAnalyticSource">
      <InputProperty name="Input0">
        <DataTypeDomain name="input_type">
          <DataType value="vtkImageData" />
        </DataTypeDomain>
      </InputProperty>
      <IntVectorProperty command="SetWholeExtent"
                         default_values="-10 10 -10 10 -10 10"
                         label="Whole Extent"
                         name="WholeExtent"
                         number_of_elements="6"
                         panel_visibility="default">
        <IntRangeDomain name="range" />
      </IntVectorProperty>
      <Hints>
        <MultiplexerSourceProxy proxygroup="filters" proxyname="Multiplexer">
          <LinkProperties>
            <!-- adding custom property linking -->
            <Property name="Input0" with_property="Input" />
          </LinkProperties>
        </MultiplexerSourceProxy>
      </Hints>
    </SourceProxy>
  </ProxyGroup>
</ServerManagerConfiguration>
"""

smtesting.ProcessCommandLineArguments()
pluginfilename = os.path.join(smtesting.TempDir, "TestMultiplexerSourceProxyPlugin.xml")
with open(pluginfilename, 'w') as f:
    f.write(xml)

LoadPlugin(pluginfilename, ns=globals())

sphere = Sphere()
mux0 = Multiplexer(Input=sphere)
mux0.Center = [1, 1, 1]
try:
    mux0.WholeExtent =[0, 1, 0, 1, 0, 1]
except AttributeError:
    pass
else:
    raise RuntimeError("'WholeExtent' should not be present")

wavelet = Wavelet()
mux1 = Multiplexer(Input=wavelet)
mux1.WholeExtent =[0, 1, 0, 1, 0, 1]
try:
    mux1.Center = [1, 1, 1]
except AttributeError:
    pass
else:
    raise RuntimeError("'Center' should not be present")

print(mux0.SMProxy)
print(mux1.SMProxy)
