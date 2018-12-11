from paraview.simple import *
from paraview import smtesting

import sys
import os
import os.path

smtesting.ProcessCommandLineArguments()

pluginXML = os.path.join(smtesting.DataDir, "Testing/Data/TestRepresentationTypePlugin.xml")
LoadPlugin(pluginXML, ns=globals())

SphereSpecial()
CreateRenderView()
r = Show()
assert r.Representation == "Points"

CreateComparativeRenderView()
r = Show()
assert r.Representation == 'Wireframe'
