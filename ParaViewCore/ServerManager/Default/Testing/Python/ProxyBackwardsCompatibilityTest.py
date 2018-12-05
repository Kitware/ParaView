from paraview.simple import *
import paraview

sourceDs = Wavelet()
inputDs = Sphere()

# Test 'Resample With Dataset` proxies
p1 = ResampleWithDataset(SourceDataArrays=inputDs, DestinationMesh=sourceDs)
print("Proxy Name before: %s" % p1.GetXMLName())
assert p1.GetXMLName() == "ResampleWithDataset", "The default proxy name must be ResampleWithDataset"

print("")
print("Setting compatibility version to 5.0...")
paraview.compatibility.major = 5
paraview.compatibility.minor = 0

p2 = paraview.simple.ResampleWithDataset(Input=inputDs, Source=sourceDs)
print("Proxy Name for compatibility version 5.0: %s" % p2.GetXMLName())
assert p2.GetXMLName() == "Probe", "The default proxy name must be Probe"

renderView1 = GetActiveViewOrCreate('RenderView')

sphere1Display = Show(sourceDs, renderView1)

# change representation type
sphere1Display.SetRepresentationType('Point Gaussian')

# Properties modified on sphere1Display
sphere1Display.ShaderPreset = 'Gaussian Blur (Default)'

# reset view to fit data
renderView1.ResetCamera()

# update the view to ensure updated data information
renderView1.Update()
