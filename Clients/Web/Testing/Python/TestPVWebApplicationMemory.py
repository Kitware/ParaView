#### import the simple module from the paraview
from paraview import simple

from paraview.modules.vtkPVClientWeb import vtkPVWebApplication

view = simple.CreateRenderView()

wavelet = simple.Wavelet()
simple.Show()
simple.Render(view)

webApp = vtkPVWebApplication()
#  This should NOT report leaks when compiling with VTK_DEBUG_LEAKS
webApp.StillRender(view.SMProxy)
