from paraview.simple import *

# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")

def catalyst_execute(info):
    global producer
    producer.UpdatePipeline()
    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", producer.PointData["velocity"].GetRange(-1))
    print("pressure-range:", producer.CellData["pressure"].GetRange(0))

    # access the node pass through catalyst_execute from the simulation
    # make sure that CATALYST_PYTHONPATH is in your PYTHONPATH
    node = info.catalyst_params
    print(f"{node=}")

    ######################################
    # This is only for testing
    # validate that some of the expected fields are part of the conduit node
    assert node.has_path("catalyst/state")
    assert node.has_path("catalyst/channels/grid")
    assert node["catalyst/channels/grid/data/topologies/mesh/type"] ==  "unstructured"
    assert node["catalyst/channels/grid/data/fields"][0].name() == "velocity"
    assert node["catalyst/channels/grid/data/fields"][1].name() == "pressure"

    # validate that the node comes from this timestep
    assert node["catalyst/state/timestep"] == info.timestep
    ######################################
