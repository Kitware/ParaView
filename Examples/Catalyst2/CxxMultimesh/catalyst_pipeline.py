from paraview.simple import *

# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="input")

def catalyst_execute(info):
    global grid, particles
    producer.UpdatePipeline()
    print(producer.GetDataInformation().GetDataAssembly())
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
    print("  author:",
            gridInfo.GetFieldDataInformation().GetArrayInformation("author").GetRangesAsString())
    print("  mesh time:",
            gridInfo.GetFieldDataInformation().GetArrayInformation("mesh time").GetComponentRange(0))
    print("  mesh timestep:",
            gridInfo.GetFieldDataInformation().GetArrayInformation("mesh timestep").GetComponentRange(0))
    print("  mesh external data:",
            gridInfo.GetFieldDataInformation().GetArrayInformation("mesh external data").GetComponentRange(0))

    print("particles:")
    print("  bounds:", particlesInfo.GetBounds())

    # try subsetting using the assembly and confirm it's as exepected.
    gridInfo2 = producer.GetSubsetDataInformation(0, "//Grid", "Assembly")
    assert gridInfo2.GetNumberOfPoints() == gridInfo.GetNumberOfPoints()

    info2 = producer.GetSubsetDataInformation(0, "//SubCollection", "Assembly")
    assert producer.GetDataInformation().GetNumberOfDataSets() == info2.GetNumberOfDataSets()
