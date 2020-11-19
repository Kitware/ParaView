# script-version: 1.0

def RequestDataDescription(datadescription):
    for i in range(datadescription.GetNumberOfInputDescriptions()):
        datadescription.GetInputDescription(i).AllFieldsOn()
        datadescription.GetInputDescription(i).GenerateMeshOn()

last_time = None
def DoCoProcessing(datadescription):
    global last_time
    if last_time is not None:
        assert last_time != datadescription.GetTime()
    last_time = datadescription.GetTime()
