from paraview.simple import *

import paraview.catalyst as catalyst

from mpi4py import MPI  # we only use it to make the output neater

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# print values for parameters passed via adaptor (note these don't change,
# and hence must be created as command line params)
print("executing catalyst_pipeline")
print("===================================")
print("script args={}".format(catalyst.get_args()))
print("===================================")

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="${args.channel-name}")

extractor = CreateExtractor("VTPD", producer, registrationName="VTPD")
extractor.Writer.FileName = "data_{timestep:06d}.vtpd"

# this is only for testing purposes. never directly import `catalyst.detail`
from paraview.catalyst.detail import IsInsituInput, _transform_registration_name

assert IsInsituInput("${args.channel-name}")
assert _transform_registration_name("${args.channel-name}") == "grid"

# ------------------------------------------------------------------------------
# Catalyst options
options = catalyst.Options()
# the pipeline with be executed evey two timesteps
options.GlobalTrigger.Frequency = 2
# Enable live visualization. To use it in ParaView click Catalyst > Connect prior to running
# this script.
options.EnableCatalystLive = 1


def catalyst_execute(info):
    global producer

    producer.UpdatePipeline()

    output = f"------------------- rank {rank}/{size} ----------------------\n"
    output += "executing (cycle={}, time={})\n".format(info.cycle, info.time)
    output += "bounds: {}\n".format(producer.GetDataInformation().GetBounds())
    output += "psi01-u-range: {}\n".format(producer.PointData["psi01"].GetRange(0))
    output += "psi01-v-range: {}\n".format(producer.PointData["psi01"].GetRange(1))
    output += "psi01-magnitute-range: {} \n".format(
        producer.PointData["psi01"].GetRange(-1)
    )
    output += "------------------------------------------------------------\n"
    print(output)
