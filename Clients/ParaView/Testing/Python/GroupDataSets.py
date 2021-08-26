from paraview.simple import *

s = Sphere(registrationName="Sphere")
w = Wavelet(registrationName="Wavelet")
g = GroupDatasets(Input=[s, w])

# Confirm that the defaults were initialized as expected.
assert g.BlockNames == ["Sphere", "Wavelet"]
g.UpdatePipeline()

dataInfo = g.GetDataInformation()
assert dataInfo.GetBlockName(1) == "Sphere" and dataInfo.GetBlockName(2) == "Wavelet"

# change block names
g.BlockNames = ["Random"]
g.UpdatePipeline()

dataInfo = g.GetDataInformation()
assert dataInfo.GetBlockName(1) == "Random" and dataInfo.GetBlockName(2) == "Block 1"
