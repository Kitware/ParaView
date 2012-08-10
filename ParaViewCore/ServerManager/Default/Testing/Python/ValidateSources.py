"""This is a test to test the paraview proxy manager API."""
from paraview import servermanager

servermanager.Connect()
sources = servermanager.sources.__dict__

for source in sources:
  try:
    s = sources[source]()
    s.UpdateVTKObjects()
  except Exception, e:
    print "Error creating:", str(s)
