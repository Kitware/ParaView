"""
Test servermanager.Fetch with composite input
"""
import paraview
from paraview.simple import *

def main():
    sphere1 = Sphere()

    # Test vtkPartitionedDataSetCollection output
    axisAlignedReflectionFilter1 = AxisAlignedReflectionFilter(Input=sphere1)
    axisAlignedReflectionFilter1.Set(
        PlaneMode='Y Min',
    )
    axisAlignedReflectionFilter1.UpdatePipeline()

    info = paraview.servermanager.Fetch(axisAlignedReflectionFilter1)
    assert info.GetClassName() == "vtkPartitionedDataSetCollection"

    # Test vtkMultiBlockDataSet output
    groupDatasets1 = GroupDatasets(Input=sphere1)
    groupDatasets2 = GroupDatasets(Input=groupDatasets1)

    groupDatasets2.UpdatePipeline()

    info = paraview.servermanager.Fetch(groupDatasets2)
    assert info.GetBlock(0).GetClassName() == "vtkMultiBlockDataSet"

    groupDatasets2.CombineFirstLayerMultiblock = True
    groupDatasets2.UpdatePipeline()

    info = paraview.servermanager.Fetch(groupDatasets2)
    assert info.GetBlock(0).GetClassName() == "vtkPolyData"


if __name__ == "__main__":
    main()
