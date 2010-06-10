""" This module can be used to run a simple rendering benchmark test. This
test renders a sphere with various rendering settings and reports the rendering
rate achieved in triangles/sec. """

import time
import sys
from paraview.simple import *

def render(ss, v, title, nframes):
  print '============================================================'
  print title
  res = []
  res.append(title)
  for phires in (500, 1000):
    ss.PhiResolution = phires
    c = v.GetActiveCamera()
    v.CameraPosition = [-3, 0, 0]
    v.CameraFocalPoint = [0, 0, 0]
    v.CameraViewUp = [0, 0, 1]
    Render()
    c1 = time.time()
    for i in range(nframes):
      c.Elevation(0.5)
      Render()
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


def run(filename=None, nframes=60):
  """ Runs the benchmark. If a filename is specified, it will write the
  results to that file as csv. The number of frames controls how many times
  a particular configuration is rendered. Higher numbers lead to more accurate
  averages. """
  # Turn off progress printing
  paraview.servermanager.SetProgressPrintingEnabled(0)
  
  # Create a sphere source to use in the benchmarks
  ss = Sphere(ThetaResolution=1000, PhiResolution=500)
  rep = Show()
  v = Render()
  
  results = []

  # Start with these defaults

  #v.RemoteRenderThreshold = 0
  v.UseImmediateMode = 0
  v.UseTriangleStrips = 0
  
  # Test different configurations
  v.UseImmediateMode = 0
  title = 'display lists, no triangle strips, solid color'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title, nframes))
  
  title = 'display lists, triangle strips, solid color'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title, nframes))
  
  v.UseImmediateMode = 1
  title = 'no display lists, no triangle strips, solid color'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title, nframes))
  
  title = 'no display lists, triangle strips, solid color'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title, nframes))

  # Color by normals
  lt = servermanager.rendering.PVLookupTable()
  rep.LookupTable = lt
  rep.ColorAttributeType = 0 # point data
  rep.ColorArrayName = "Normals"
  lt.RGBPoints = [-1, 0, 0, 1, 0.0288, 1, 0, 0]
  lt.ColorSpace = 'HSV'
  lt.VectorComponent = 0
  
  v.UseImmediateMode = 0
  title = 'display lists, no triangle strips, color by array'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title, nframes))

  title = 'display lists, triangle strips, color by array'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title, nframes))
  v.UseImmediateMode = 1

  v.UseImmediateMode = 1
  title = 'no display lists, no triangle strips, color by array'
  v.UseTriangleStrips = 0
  results.append(render(ss, v, title, nframes))

  title = 'no display lists, triangle strips, color by array'
  v.UseTriangleStrips = 1
  results.append(render(ss, v, title, nframes))

  if filename:
    f = open(filename, "w")
  else:
    f = sys.stdout
  print >>f, 'configuration, %d, %d' % (results[0][1][0], results[0][2][0])
  for i in results:
    print >>f, '"%s", %g, %g' % (i[0], i[1][1], i[2][1])  

if __name__ == "__main__":
  run()
