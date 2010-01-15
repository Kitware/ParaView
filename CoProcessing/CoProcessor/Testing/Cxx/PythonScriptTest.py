def DoCoProcessing(datadescription):
  timestep = datadescription.GetTimeStep()
    
  grid = datadescription.GetInputDescription("input").GetGrid()
  pressure = grid.GetPointData().GetArray('Pressure')

  grid.GetPointData().SetScalars(pressure)
  obj.SetOutput(grid)

  # get global range of Pressure
  di = trivialproducer.GetDataInformation(0)
  trivialproducer.UpdatePipeline()
  di.Update()
  pdi = di.GetPointDataInformation()
  ai = pdi.GetArrayInformation('Pressure')
  pressurerange = ai.GetComponentRange(0)
  
  contour.Isosurfaces = .5*(pressurerange[0]+pressurerange[1])

  # now output the results to the screen as well as taking
  # a screen shot of the view
  #setup a window
  rep = Show(contour)
  ren = Render()
 
  #set the background color
  ren.Background=[1,1,1]  #white
 
  #set image size
  ren.ViewSize = [200, 300] #[width, height]
 
  #set representation
  rep.Representation="Surface"
 
  #save screenshot
  pressureimagefilename = 'CPPressure'+str(timestep) + '.png'
  WriteImage(pressureimagefilename)

  rep = Show(trivialproducer)
  rep.LookupTable = MakeBlueToRedLT(pressurerange[0], pressurerange[1])
  rep.ColorArrayName = 'Pressure'
  rep.ColorAttributeType = 'POINT_DATA'
  #set representation
  rep.Representation="Surface"
  rep = Show(contour)
  #set the background color
  ren = Render()
  ren.Background=[1,1,1]  #white

  gridimagefilename = 'CPGrid'+str(timestep) + '.png'
  WriteImage(gridimagefilename)
  
  return

def RequestDataDescription(datadescription):
  time = datadescription.GetTime()
  timestep = datadescription.GetTimeStep()
  if timestep % 20 == 0:
    # add in some fields
    #print 'added Pressure and wanting to do coprocessing'
    datadescription.GetInputDescription("input").AddPointField("Pressure")
    datadescription.GetInputDescription('input').GenerateMeshOn()
  return

# the code below is needed to import objects from paraview.simple
# plus the definition of vtkTrivialProducer into this python script.
try: paraview.simple
except: from paraview.simple import *

trivialproducer = TrivialProducer()
contour = Contour(Input=trivialproducer)
obj = trivialproducer.GetClientSideObject()


