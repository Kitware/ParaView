#### import the simple module from the paraview
from paraview.simple import *

# create 2 wavelets
wavelet1 = Wavelet()
wavelet2 = Wavelet()

composite = GroupDatasets(Input=[wavelet1, wavelet2])

# We test if the ghost cells generator can be instantiated for composite data sets as well as
# data sets
generator1 = GhostCellsGenerator(Input=composite)
generator2 = GhostCellsGenerator(Input=wavelet1)
