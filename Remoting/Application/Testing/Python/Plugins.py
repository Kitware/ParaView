# Simply loads a couple of plugins to ensure that definitions get updated
# correctly.

from paraview.simple import *
from paraview import print_error, print_info
import sys
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

LoadDistributedPlugin('SurfaceLIC', ns=globals())

filename = smtesting.DataDir + '/Testing/Data/disk_out_ref.ex2'
data = OpenDataFile(filename)
UpdatePipeline()
view = CreateRenderView()
rep = Show(view=view, representationType="UnstructuredGridRepresentation")

# Ensure that loading the SurfaceLIC lead to adding the SelectLICVectors
# property correctly.
print_info(rep.GetProperty("SelectInputVectors"))
print_info(rep.SelectInputVectors)

try:
    LoadDistributedPlugin("NonExistantPluginName", ns=globals())
    print_error("Error: LoadDistributedPlugin should have thrown a RunTimeError!!!")
    sys.exit(1)
except:
    # We expect an error.
    pass

try:
    MomentVectors()
    print_error("Error: MomentVectors should not exist before loading the plugin")
    sys.exit(1)
except:
    pass

LoadDistributedPlugin('Moments', ns=globals())
MomentVectors()
