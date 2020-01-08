from paraview.simple import *
from paraview import smtesting
import os.path

smtesting.ProcessCommandLineArguments()
pluginXML = os.path.join(smtesting.DataDir, "Testing/Data/PythonProgrammableFilterParameters.xml")

LoadPlugin(pluginXML, ns=globals())

s = Sample()
s.Filenames = ["alpha", "beta"]
s.UpdatePipeline()
assert got_filenames == ["alpha", "beta"]

s.Filenames = ["gamma"]
s.UpdatePipeline()
assert got_filenames == ["gamma"]
