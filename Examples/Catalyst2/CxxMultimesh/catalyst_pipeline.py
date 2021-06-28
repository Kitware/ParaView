from paraview.simple import *

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="input")

def catalyst_execute(info):
    global grid, particles
    producer.UpdatePipeline()
    assert producer.GetDataInformation().GetNumberOfDataSets() == 2

    gridInfo = producer.GetSubsetDataInformation(0, "//grid", "Hierarchy");
    particlesInfo = producer.GetSubsetDataInformation(0, "//particles", "Hierarchy");

    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("grid:")
    print("  bounds:", gridInfo.GetBounds())
    print("  velocity-range:",
            gridInfo.GetPointDataInformation().GetArrayInformation("velocity").GetComponentRange(0))
    print("  pressure-range:",
            gridInfo.GetCellDataInformation().GetArrayInformation("pressure").GetComponentRange(0))

    print("particles:")
    print("  bounds:", particlesInfo.GetBounds())
