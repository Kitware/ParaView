from paraview.simple import *

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
grid = TrivialProducer(registrationName="grid")
particles = TrivialProducer(registrationName="particles")

def catalyst_execute(info):
    global grid, particles
    grid.UpdatePipeline()
    particles.UpdatePipeline()

    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("grid:")
    print("  bounds:", grid.GetDataInformation().GetBounds())
    print("  velocity-range:", grid.PointData["velocity"].GetRange())
    print("  pressure-range:", grid .CellData["pressure"].GetRange(0))

    print("particles:")
    print("  bounds:", particles.GetDataInformation().GetBounds())
