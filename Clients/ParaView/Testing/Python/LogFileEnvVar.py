from paraview import print_info
from paraview.vtk.util.misc import vtkGetTempDir
from time import time

log_str = "This is a test message "+str(time())
print_info(log_str)
file = open(vtkGetTempDir()+"/pv_envvar.log", "r")
assert log_str in file.read()
