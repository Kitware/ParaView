# Simply loads a couple of plugins to ensure that definitions get updated
# correctly.

from paraview.simple import *
import sys
import SMPythonTesting
SMPythonTesting.ProcessCommandLineArguments()

LoadDistributedPlugin('SLACTools', False, globals())
LoadDistributedPlugin('SurfaceLIC', False, globals())

filename = SMPythonTesting.DataDir + '/Data/disk_out_ref.ex2'
data = OpenDataFile(filename)
rep = Show()

# Ensure that loading the SurfaceLIC lead to adding the SelectLICVectors
# property correctly.
print rep.GetProperty("SelectLICVectors")
print rep.SelectLICVectors

try:
    LoadDistributedPlugin("NonExistantPluginName", False, globals())
    print "Error: LoadDistributedPlugin should have thrown a RunTimeError!!!"
    sys.exit(1)
except:
    # We expect an error.
    pass

try:
    MomentVectors()
    print "Error: MomentVectors should not exist before loading the plugin"
    sys.exit(1)
except:
    pass

LoadDistributedPlugin('Moments', False, globals())
MomentVectors()
