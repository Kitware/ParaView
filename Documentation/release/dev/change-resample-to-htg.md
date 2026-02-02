## **Resample To Hyper Tree Grid** filter can produce composite datasets

If a composite dataset is given in input, it now copies its structure in the output and resample each dataset of the composite input to an hyper tree grid.

### Developer notes

In `vtkResampleToHyperTreeGrid.h` :
- The following struct have been moved from `protected` visibility to `private` : `GridElement`, `PriorityQueueElement`
- The following methods have been moved from `protected` visibility to `private` : `ProcessTrees`, `GenerateTrees`, `IndexToMultiResGridCoordinates`, `IndexToGridCoordinates`, `MultiResGridCoordinatesToIndex`, `GridCoordinatesToIndex`, `BroadcastHyperTreeOwnership`, `RecursivelyFillGaps`, `ExtrapolateValuesOnGaps`, `RecursivelyFillPriorityQueue`, `SubdivideLeaves`, `CreateGridOfMultiResolutionGrids`, `IntersectedVolume`
- The following variables have been moved from `protected` visibility to `private` : `BranchFactor`, `Dimensions`, `MaxDepth`, `Progress`, `Mask`, `CellDims`, `NumberOfChildren`, `NumberOfLeavesInSubtreeField`, `NumberOfPointsInSubtreeField`, `MinimumNumberOfPointsInSubtree`, `MaxResolutionPerTree`, `Extrapolate`, `NoEmptyCells`, `ResolutionPerTree`, `Diagonal`, `GridOfMultiResolutionGrids`, `Bounds`, `LocalHyperTreeBoundingBox`, `Controller`
- The following variables have been removed : `MaxCache`, `MinCache`, `InputDataArrayNames`
In `vtkResampleToHyperTreeGrid.h` and `vtkResampleToHyperTreeGrid.cxx` :
- The following variables have been renamed and moved from `protected` visibility to `private` : `Min` in `LowerThreshold`, `Max` in `UpperThreshold`, `InRange` in `InvertRange` (and its role has been inverted), `ArrayMeasurement` in `SubdivisionMethod`, `ArrayMeasurementDisplay` in `InterpolationMethod`, `InputPointDataArrays` in `InputArrays`, `ArrayMeasurements` in `ArrayValuesAccumulators`
