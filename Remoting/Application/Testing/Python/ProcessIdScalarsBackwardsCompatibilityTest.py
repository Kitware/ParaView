import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 14

from paraview.simple import *

wavelet = Wavelet()

processIdScalars = ProcessIdScalars(Input=wavelet)
assert(type(processIdScalars).__name__ == "ProcessIds")
