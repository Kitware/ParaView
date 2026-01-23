## Update **Resample To Hyper Tree Grid filter**

### Supports cell data

- The filter now supports subdivisions using a criterion based on cell data values.

### Improve UI

- The UI was redesigned : **No Empty Cells**, **Interpolation Method**, **Minimum Number Of Points In Subtree** and **Extrapolate Point Data** options has been moved to advanced panel, similar parameters are now grouped together and the threshold selection for subdivision is now similar to the **Threshold** filter.
- The selection of arrays to interpolate was removed : if an interpolation method is set, all arrays of the same type (point or cell) than the subdivision array are now interpolated.

### Change output for composite datasets

- If a composite dataset is given in input, it now copies its structure in the output and resample each dataset of the composite input to an hyper tree grid.

### Developer notes

In `vtkResampleToHyperTreeGrid.h` :
- The following struct have been moved from `protected` visibility to `private` : `GridElement`, `PriorityQueueElement`
- The following methods have been moved from `protected` visibility to `private` : `ProcessTrees`, `GenerateTrees`, `IndexToMultiResGridCoordinates`, `IndexToGridCoordinates`, `MultiResGridCoordinatesToIndex`, `GridCoordinatesToIndex`, `BroadcastHyperTreeOwnership`, `RecursivelyFillGaps`, `ExtrapolateValuesOnGaps`, `RecursivelyFillPriorityQueue`, `SubdivideLeaves`, `CreateGridOfMultiResolutionGrids`, `IntersectedVolume`
- The following variables have been moved from `protected` visibility to `private` : `BranchFactor`, `Dimensions`, `MaxDepth`, `Progress`, `Mask`, `CellDims`, `NumberOfChildren`, `NumberOfLeavesInSubtreeField`, `NumberOfPointsInSubtreeField`, `MinimumNumberOfPointsInSubtree`, `MaxResolutionPerTree`, `Extrapolate`, `NoEmptyCells`, `ResolutionPerTree`, `Diagonal`, `GridOfMultiResolutionGrids`, `Bounds`, `LocalHyperTreeBoundingBox`, `Controller`
- The following variables have been removed : `MaxCache`, `MinCache`, `InputDataArrayNames`
In `vtkResampleToHyperTreeGrid.h` and `vtkResampleToHyperTreeGrid.cxx` :
- The following variables have been renamed and moved from `protected` visibility to `private` : `Min` in `LowerThreshold`, `Max` in `UpperThreshold`, `InRange` in `InvertRange` (and its role has been inverted), `ArrayMeasurement` in `SubdivisionMethod`, `ArrayMeasurementDisplay` in `InterpolationMethod`, `InputPointDataArrays` in `InputArrays`, `ArrayMeasurements` in `ArrayValuesAccumulators`
