
from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot

from os.path import join

datadir = join(vtkGetDataRoot(), "Testing/Data")

cgnsFile = join(datadir, "channelBump_solution.cgns")

reader = CGNSSeriesReader(FileNames=[cgnsFile])
convert = ConvertPolyhedralCells(Input=reader)
convert.UpdatePipeline()
