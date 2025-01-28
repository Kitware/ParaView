"""
Test servermanager.Fetch with composite input
"""
import paraview
from paraview.simple import *

def main():
    sphere1 = Sphere()

    axisAlignedReflectionFilter1 = AxisAlignedReflectionFilter(Input=sphere1)
    axisAlignedReflectionFilter1.Set(
        PlaneMode='Y Min',
    )
    axisAlignedReflectionFilter1.UpdatePipeline()

    info = paraview.servermanager.Fetch(axisAlignedReflectionFilter1)
    assert info.GetClassName() == "vtkPartitionedDataSetCollection"

if __name__ == "__main__":
    main()
