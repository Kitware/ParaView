from paraview.simple import *
import paraview.vtk.util.misc
import paraview.util

def PrintDiff(testFiles, fileNames):
    for (item1, item2) in zip(testFiles, fileNames):
        if item1 != item2:
            print(item1, " != ", item2)

root = paraview.vtk.util.misc.vtkGetDataRoot() + "/Testing/Data/globbing/"
name = "glob*.png"
testFiles = [root + "glob0.png", root + "glob1.png", root + "glob2.png", root + "glob3.png"]

glob = paraview.util.Glob(root + name)

PrintDiff(testFiles, glob)

if glob != testFiles:
    raise RuntimeError("Globbing failed")
