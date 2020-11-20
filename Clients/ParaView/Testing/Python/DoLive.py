# add comment to indicate version number for the script.
# script-version: 2.0

# A simple Catalyst analysis script that simple
# connects to ParaView GUI via Catalyst Live

#--------------------------------------
# catalyst options
from paraview import catalyst
options = catalyst.Options()
options.EnableCatalystLive = 1
options.CatalystLiveTrigger = 'TimeStep'
options.CatalystLiveURL = "localhost:22222"

#--------------------------------------
# List individual modules with Catalyst analysis scripts
scripts = []
