# script-version: 2.0
from paraview.simple import *
from paraview import catalyst
import time

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [1600,800]
renderView1.CameraPosition = [157.90070691620653, 64.91180236667495, 167.90421495515105]
renderView1.CameraFocalPoint = [19.452526958533134, 28.491610229010647, 10.883993417012459]
renderView1.CameraViewUp = [0.07934883419275315, 0.953396338566962, -0.2910999555468221]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 54.99504523136608

# get color transfer function/color map for 'velocity'
velocityLUT = GetColorTransferFunction('velocity')
velocityLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 29.205000000000002, 0.865003, 0.865003, 0.865003, 58.410000000000004, 0.705882, 0.0156863, 0.14902]
velocityLUT.ScalarRangeInitialized = 1.0

# show data from grid
gridDisplay = Show(producer, renderView1, 'UnstructuredGridRepresentation')

gridDisplay.Representation = 'Surface'
gridDisplay.ColorArrayName = ['POINTS', 'velocity']
gridDisplay.LookupTable = velocityLUT

# get color legend/bar for velocityLUT in view renderView1
velocityLUTColorBar = GetScalarBar(velocityLUT, renderView1)
velocityLUTColorBar.Title = 'velocity'
velocityLUTColorBar.ComponentTitle = 'Magnitude'

# set color bar visibility
velocityLUTColorBar.Visibility = 1

# show color legend
gridDisplay.SetScalarBarVisibility(renderView1, True)

# ----------------------------------------------------------------
# setup extractors
# ----------------------------------------------------------------

SetActiveView(renderView1)
# create extractor
pNG1 = CreateExtractor('PNG', renderView1, registrationName='PNG1')
# trace defaults for the extractor.
pNG1.Trigger = 'TimeStep'

# init the 'PNG' selected for 'Writer'
pNG1.Writer.FileName = 'screenshot_{timestep:06d}.png'
pNG1.Writer.ImageResolution = [1600,800]
pNG1.Writer.Format = 'PNG'

# ------------------------------------------------------------------------------
# Catalyst options
options = catalyst.Options()
if "--enable-live" in catalyst.get_args():
  options.EnableCatalystLive = 1


# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline")
def catalyst_execute(info):
    global producer
    producer.UpdatePipeline()
    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", producer.PointData["velocity"].GetRange(-1))
    print("pressure-range:", producer.CellData["pressure"].GetRange(0))
    # In a real simulation sleep is not needed. We use it here to slow down the
    # "simulation" and make sure ParaView client can catch up with the produced
    # results instead of having all of them flashing at once.
    if options.EnableCatalystLive:
        time.sleep(1)
