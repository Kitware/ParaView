from paraview.simple import *

# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline")

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

    global steerable_parameters

    # Emulate user modification of the Proxy:
    steerable_parameters.Center[0] = info.time
    steerable_parameters.Center[1] = info.time
    steerable_parameters.Center[2] = info.time
    steerable_parameters.Type[0] = int(info.time * 10) % 3
