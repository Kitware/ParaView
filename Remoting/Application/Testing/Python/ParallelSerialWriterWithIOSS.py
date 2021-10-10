from paraview import smtesting
from paraview.simple import *
from paraview import print_info

smtesting.ProcessCommandLineArguments()
fnames = ["can.e.4.0", "can.e.4.1", "can.e.4.2", "can.e.4.3"]
fnames = [ "%s/Testing/Data/can.e.4/%s" % (smtesting.DataDir, x) for x in fnames]

reader = OpenDataFile(fnames)
extractSurface = ExtractSurface(reader)
extractSurface.UpdatePipeline()

tempDir = smtesting.GetUniqueTempDirectory(smtesting.TempDir + "/ParallelSerialWriterWithIOSS-")
print_info("Generating results in '%s'", tempDir)
SaveData(tempDir + "/can.stl", extractSurface)

block0 = OpenDataFile(tempDir +"/can0.stl")
block1 = OpenDataFile(tempDir +"/can1.stl")
GroupDatasets(Input=[block0, block1])
Show()
view = Render()
camera = GetActiveCamera()
camera.Azimuth(90)

smtesting.DoRegressionTesting(view.SMProxy)
if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')
