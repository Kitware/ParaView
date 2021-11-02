from paraview.simple import *
import paraview.vtk.util.misc

def PrintDiff(testFiles, fileNames):
    for (item1, item2) in zip(testFiles, fileNames):
        if item1 != item2:
            print(item1, " != ", item2)

root = paraview.vtk.util.misc.vtkGetDataRoot() + "/Testing/Data/globbing/"
testFiles = [root + "foo_a.png", root + "foo_b.png", root + "glob_a.png", root + "glob_b.png", root
        + "glob_c.png", root + "glob_d.png"]

reader = PNGSeriesReader(FileNames = root + "glob_a.png")

reader.FileNames = [root + "foo*.png", root + "glob*.png"]
PrintDiff(testFiles, reader.FileNames)
if reader.FileNames != testFiles:
    raise RuntimeError("Globbing failed when input list has 2 items")

reader.FileNames = [root + "*.png"]
PrintDiff(testFiles, reader.FileNames)
if reader.FileNames != testFiles:
    raise RuntimeError("Globbing failed when input list has 1 item")
