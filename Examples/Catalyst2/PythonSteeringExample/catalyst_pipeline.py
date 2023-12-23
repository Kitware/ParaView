from paraview.simple import *

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="input")


def catalyst_execute(info):
    global producer

    producer.UpdatePipeline()

    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("-----")
    print("-----")
    print("bounds:", producer.GetDataInformation().GetBounds())
    print("available point arrays:", producer.PointData.keys())
    print("available cell arrays:", producer.CellData.keys())

    if info.timestep > 0:
      if info.timestep % 2 == 0:
        assert "pressure" in producer.CellData.keys()
      else:
        assert "pressure" not in producer.CellData.keys()

    print("===================================")

def catalyst_results(info):
    info.catalyst_params['input/request/velocity'] = True
    if (info.timestep + 1)% 2 == 0:
      info.catalyst_params['input/request/pressure'] = True
    else:
      info.catalyst_params['input/request/pressure'] = False
