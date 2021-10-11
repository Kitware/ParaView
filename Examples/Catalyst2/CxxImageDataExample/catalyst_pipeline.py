from paraview.simple import *

from paraview.catalyst import get_args, get_execute_params

# print values for parameters passed via adaptor (note these don't change,
# and hence must be created as command line params)
print("executing catalyst_pipeline")
print("===================================")
print("pipeline args={}".format(get_args()))
print("===================================")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="${args.channel-name}")


# this is only for testing purposes. never directly import `catalyst.detail`
from paraview.catalyst.detail import IsInsituInput, _transform_registration_name
assert(IsInsituInput("${args.channel-name}"))
assert(_transform_registration_name("${args.channel-name}")=="grid")

def catalyst_execute(info):
    global producer

    producer.UpdatePipeline()

    # get time parameter as example of a parameter changing during the simulation
    params = get_execute_params()
    timeParam = float(params[3].split("=")[1])

    # show that the time parameter matches the time tracked by catalyst
    assert (timeParam - info.time) < 1e-12

    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("-----")
    print("pipeline parameters:")
    print("\n".join(params))
    print("-----")
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", producer.PointData["velocity"].GetRange(-1))
    print("pressure-range:", producer.CellData["pressure"].GetRange(0))
    print("===================================")
