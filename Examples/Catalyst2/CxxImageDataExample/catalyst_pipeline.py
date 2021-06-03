from paraview.simple import *

from paraview.catalyst import get_args, get_execute_params

# print values for parameters passed via adaptor (note these don't change,
# and hence must be created as command line params)
print("===================================")
print("pipeline args={}".format(get_args()))
print("===================================")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")

def catalyst_execute(info):
    global producer

    producer.UpdatePipeline()
    print("-----------------------------------")
    print("===================================")
    print("pipeline parameters:")
    print("  \n".join(get_execute_params()))
    print("===================================")

    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", producer.PointData["velocity"].GetRange(-1))
    print("pressure-range:", producer.CellData["pressure"].GetRange(0))
