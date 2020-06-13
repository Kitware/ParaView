# A simple Catalyst analysis script that simple
# connects to ParaView GUI via Catalyst Live

#--------------------------------------
# catalyst options
from paraview import catalyst
options = catalyst.Options()
options.EnableCatalystLive = 1
options.CatalystLiveTrigger = 'Time'
options.CatalystLiveURL = "localhost:22222"

#--------------------------------------
# List individual modules with Catalyst analysis scripts
scripts = []
