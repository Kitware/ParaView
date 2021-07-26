from paraview.simple import *

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")
steerable_parameters = CreateSteerableParameters("SteerableParameters")

from paraview import catalyst

options = catalyst.Options()
options.EnableCatalystLive = 1

def catalyst_execute(info):
    global producer
    producer.UpdatePipeline()
    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", producer.PointData["velocity"].GetRange(-1))
    print("pressure-range:", producer.CellData["pressure"].GetRange(0))
    print("timestep: ", producer.FieldData["timestep"].GetRange(0)[0])

    SaveExtractsUsingCatalystOptions(options)
