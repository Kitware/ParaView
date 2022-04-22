# This test varifies that gathering rank specific information works
# as expected. It's designed to run on 5 ranks.


from paraview.simple import *
from paraview import smtesting

pm = servermanager.vtkProcessModule.GetProcessModule()
if pm.GetNumberOfLocalPartitions() != 5:
    raise smtesting.TestError("Test must be run on 5 ranks!")
if pm.GetSymmetricMPIMode():
    raise smtesting.TestError("Test cannot be run in symmetric mode!")

s = Sphere()
s.UpdatePipeline()

bds = s.GetRankDataInformation(0).GetBounds()
strBds = "({:1.1f}, {:1.1f}, {:1.1f}, {:1.1f}, {:1.1f}, {:1.1f})".format(bds[0], bds[1], bds[2], bds[3], bds[4], bds[5])
print("rank 0: ", strBds)
assert "(0.0, 0.5, 0.0, 0.3, -0.5, 0.5)" == strBds

bds = s.GetRankDataInformation(3).GetBounds()
strBds = "({:1.1f}, {:1.1f}, {:1.1f}, {:1.1f}, {:1.1f}, {:1.1f})".format(bds[0], bds[1], bds[2], bds[3], bds[4], bds[5])
print("rank 3: ", strBds)
assert "(-0.5, 0.0, -0.5, 0.0, -0.5, 0.5)" == strBds
