# Simply loads a couple of plugins to ensure that definitions get updated
# correctly.

from paraview.simple import *
import sys
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

LoadDistributedPlugin('SLACTools', False, globals())
LoadDistributedPlugin('SurfaceLIC', False, globals())

filename = smtesting.DataDir + '/Testing/Data/disk_out_ref.ex2'
data = OpenDataFile(filename)
rep = Show()

# Ensure that loading the SurfaceLIC lead to adding the SelectLICVectors
# property correctly.
print(rep.GetProperty("SelectInputVectors"))
print(rep.SelectInputVectors)

try:
    LoadDistributedPlugin("NonExistantPluginName", False, globals())
    print("Error: LoadDistributedPlugin should have thrown a RunTimeError!!!")
    sys.exit(1)
except:
    # We expect an error.
    pass

try:
    MomentVectors()
    print("Error: MomentVectors should not exist before loading the plugin")
    sys.exit(1)
except:
    pass

LoadDistributedPlugin('Moments', False, globals())
MomentVectors()
