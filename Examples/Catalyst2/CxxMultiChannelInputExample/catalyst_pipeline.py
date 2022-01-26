from paraview.simple import *

# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
grid = TrivialProducer(registrationName="grid")
particles = TrivialProducer(registrationName="particles")

def catalyst_execute(info):
    global grid, particles
    grid.UpdatePipeline()
    particles.UpdatePipeline()

    # test that the particles time lags behind the mesh's
    gridTime = grid.GetDataInformation().GetTime()
    particleTime = particles.GetDataInformation().GetTime()
    particleTimeTest = gridTime - (info.cycle % 2)*0.1
    assert(abs(particleTimeTest - particleTime) < 1e-12)

    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("grid:")
    print("  type:", grid.GetDataInformation().GetDataSetTypeAsString())
    print("  time:", gridTime)
    print("  bounds:", grid.GetDataInformation().GetBounds())
    print("  velocity-range:", grid.PointData["velocity"].GetRange())
    print("  pressure-range:", grid .CellData["pressure"].GetRange(0))

    print("particles:")
    print("  type:", particles.GetDataInformation().GetDataSetTypeAsString())
    print("  time:", particleTime)
    print("  bounds:", particles.GetDataInformation().GetBounds())
