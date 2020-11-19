# script-version: 2.0

# Test confirms that scripts with just the live triggers
# are handled correctly by Catalyst


#--------------------------------------
# catalyst options
from paraview import catalyst
options = catalyst.Options()
options.EnableCatalystLive = 1
options.CatalystLiveTrigger = 'TimeStep'
options.CatalystLiveURL = "localhost:22222"


def catalyst_finalize():
    from paraview.simple import GetSources
    # since this is live-only script, ParaView should have
    # automatically created source for all in situ data sources
    # let's confirm that.
    if not GetSources():
        raise RuntimeError("No sources found!")
    else:
        print("All ok")
