"""This is a test to test the paraview proxy manager API."""
from paraview import servermanager
import sys

servermanager.Connect()
sources = servermanager.sources.__dict__

for source in sources:
  try:
    sys.stderr.write('Creating %s...'%(source))
    if source in ["GenericIOReader"]:
        print(sys.stderr.write("...skipping (in exclusion list).\n"))
        continue
    s = sources[source]()
    s.UpdateVTKObjects()
    sys.stderr.write('ok\n')
  except:
    sys.stderr.write('failed\n')
    raise RuntimeError('ERROR: Failed to create %s'%(source))
