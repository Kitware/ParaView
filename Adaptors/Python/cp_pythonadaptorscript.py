try: paraview.simple
except: from paraview.simple import *

cp_writers = []

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    timestep = datadescription.GetTimeStep()


    input_name = 'input'
    if (timestep % 2 == 0) :
        datadescription.GetInputDescriptionByName(input_name).AllFieldsOn()
        datadescription.GetInputDescriptionByName(input_name).GenerateMeshOn()
    else:
        datadescription.GetInputDescriptionByName(input_name).AllFieldsOff()
        datadescription.GetInputDescriptionByName(input_name).GenerateMeshOff()


def DoCoProcessing(datadescription):
    "Callback to do co-processing for current timestep"
    global cp_writers
    cp_writers = []
    timestep = datadescription.GetTimeStep()

    PythonAdaptorDriver_vti = CreateProducer( datadescription, "input" )

    Slice1 = Slice( guiName="Slice1", SliceOffsetValues=[0.0], SliceType="Plane" )
    Slice1.SliceType.Offset = 0.0
    Slice1.SliceType.Origin = [0.25, 0.5, 0.5]
    Slice1.SliceType.Normal = [0.0, 1.0, 0.0]

    ParallelPolyDataWriter1 = CreateWriter( XMLPPolyDataWriter, "filename_%t.pvtp", 2 )



    for writer in cp_writers:
        if timestep % writer.cpFrequency == 0:
            writer.FileName = writer.cpFileName.replace("%t", str(timestep))
            writer.UpdatePipeline()

    if timestep % 1 == 0:
        renderviews = servermanager.GetRenderViews()
        imagefilename = ""
        for view in range(len(renderviews)):
            fname = imagefilename.replace("%v", str(view))
            fname = fname.replace("%t", str(timestep))
            WriteImage(fname, renderviews[view])

    # explicitly delete the proxies -- we do it this way to avoid problems with prototypes
    tobedeleted = GetProxiesToDelete()
    while len(tobedeleted) > 0:
        Delete(tobedeleted[0])
        tobedeleted = GetProxiesToDelete()

def GetProxiesToDelete():
    iter = servermanager.vtkSMProxyIterator()
    iter.Begin()
    tobedeleted = []
    while not iter.IsAtEnd():
      if iter.GetGroup().find("prototypes") != -1:
         iter.Next()
         continue
      proxy = servermanager._getPyProxy(iter.GetProxy())
      proxygroup = iter.GetGroup()
      iter.Next()
      if proxygroup != 'timekeeper' and proxy != None and proxygroup.find("pq_helper_proxies") == -1 :
          tobedeleted.append(proxy)

    return tobedeleted

def CreateProducer(datadescription, gridname):
  "Creates a producer proxy for the grid"
  if not datadescription.GetInputDescriptionByName(gridname):
    raise RuntimeError("Simulation input name '%s' does not exist" % gridname)
  grid = datadescription.GetInputDescriptionByName(gridname).GetGrid()
  producer = TrivialProducer()
  producer.GetClientSideObject().SetOutput(grid)
  producer.UpdatePipeline()
  return producer


def CreateWriter(proxy_ctor, filename, freq):
    global cp_writers
    writer = proxy_ctor()
    writer.FileName = filename
    writer.add_attribute("cpFrequency", freq)
    writer.add_attribute("cpFileName", filename)
    cp_writers.append(writer)
    return writer
