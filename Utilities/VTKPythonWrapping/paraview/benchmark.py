import time
import sys
from paraview import servermanager

def render(ss, v, title):
  print '============================================================'
  print title
  res = []
  res.append(title)
  for phires in (500, 1000):
    ss.PhiResolution = phires
    c = v.GetActiveCamera()
    nframes = 60
    v.CameraPosition = [-3, 0, 0]
    v.CameraFocalPoints = [0, 0, 0]
    v.CameraViewUp = [0, 0, 1]
    v.StillRender()
    c1 = time.time()
    for i in range(nframes):
      c.Elevation(0.5)
      v.StillRender()
      if not servermanager.fromGUI:
        sys.stdout.write(".")
        sys.stdout.flush()
    if not servermanager.fromGUI:
      sys.stdout.write("\n")
    tpr = (time.time() - c1)/nframes
    ncells = ss.GetDataInformation().GetNumberOfCells()
    print tpr, " secs/frame"
    print ncells, " polys"
    print ncells/tpr, " polys/sec"
    
    res.append((ncells, ncells/tpr))

  return res


def run():
  # Turn off progress printing
  if servermanager.progressObserverTag:
    servermanager.ToggleProgressPrinting()
  
  # Create a sphere source to use in the benchmarks
  pm = servermanager.ProxyManager()
  ss = servermanager.sources.SphereSource(ThetaResolution=1000, PhiResolution=500, registrationGroup="sources", registrationName="benchmark source")
  
  # The view and representation
  v = servermanager.GetRenderView()
  if not v:
    v = servermanager.CreateRenderView()
  
  rep = servermanager.CreateRepresentation(ss, v, registrationGroup="representations", registrationName="benchmark rep")
  
  results = []
  
  # Test different configurations
  v.RemoteRenderThreshold = 0
  v.UseImmediateMode = 0
  title = 'display lists, no triangle strips, solid color'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title))
  
  title = 'display lists, triangle strips, solid color'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title))
  
  v.UseImmediateMode = 1
  title = 'no display lists, no triangle strips, solid color'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title))
  
  title = 'no display lists, triangle strips, solid color'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title))
  
  # Color by normals
  lt = servermanager.rendering.PVLookupTable()
  rep.LookupTable = lt
  rep.ColorAttributeType = 0 # point data
  rep.ColorArrayName = "Normals"
  lt.RGBPoints = [-1, 0, 0, 1, 0.0288, 1, 0, 0]
  lt.ColorSpace = 1 # HSV
  lt.VectorComponent = 0
  
  v.UseImmediateMode = 0
  title = 'display lists, no triangle strips, color by array'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title))

  title = 'display lists, triangle strips, color by array'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title))
  v.UseImmediateMode = 1

  title = 'no display lists, no triangle strips, color by array'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title))

  title = 'no display lists, triangle strips, color by array'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title))
  
  newr = []
  for r in v.Representations:
    if r != rep:
      newr.append(r)
  v.Representations = newr
  
  pm.UnRegisterProxy("sources", "benchmark source", ss)
  pm.UnRegisterProxy("representations", "benchmark rep", rep)
  
  ss = None
  rep = None
  
  v.StillRender()
  v = None
  
  return results

if __name__ == "__main__":
  import pickle
  p = pickle.Pickler(open("result.bench", "w"))
  servermanager.Connect()
  p.dump(run())