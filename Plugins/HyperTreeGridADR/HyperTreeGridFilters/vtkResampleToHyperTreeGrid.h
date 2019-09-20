/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleToHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkResampleToHyperTreeGrid
 * @brief   converts vtkDataSet to vtkHyperTreeGrid
 *
 * Given an input vtkDataSet with a scalar field and a subdivision strategy,
 * produces an output vtkHyperTreeGrid where the tree is shaped according to
 * the stragegy applied over the field.
 *
 * Set a pointer to an instance derived from vtkAbstractArrayMeasurement
 * with the ArrayMeasurement method to set up the subdividing strategy.
 * By default, leaves in the vtkHyperTreeGrid are divided if the measured data
 * is between GetMin() and GetMax(). If GetInRange() == false, the subdivide
 * outside of the range instead.
 *
 * Similarly use ArrayMeasurementDisplay to create a secondary criterion.
 * For example, set ArrayMeasurement to vtkStandardDeviationArrayMeasurement*
 * to produce a vtkHyperTreeGrid.
 * Set ArrayMeasurementDisplay to vtkMedianMeasurement to produce an extra
 * scalar field that is closer to the input, ie the the median of the subtree.
 *
 */

#ifndef vtkResampleToHyperTreeGrid_h
#define vtkResampleToHyperTreeGrid_h

#include "vtkAlgorithm.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro
#include "vtkTuple.h"

#include <unordered_map>
#include <vector>

class vtkAbstractArrayMeasurement;
class vtkAbstractAccumulator;
class vtkBitArray;
class vtkCell;
class vtkCell3D;
class vtkDataArray;
class vtkDataSet;
class vtkDoubleArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkInformation;
class vtkInformationVector;
class vtkLongArray;
class vtkVoxel;

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkResampleToHyperTreeGrid : public vtkAlgorithm
{
public:
  static vtkResampleToHyperTreeGrid* New();
  vtkTypeMacro(vtkResampleToHyperTreeGrid, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  virtual vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme.
   */
  vtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  vtkGetMacro(BranchFactor, unsigned int);
  //@}

  //@{
  /**
   * Set/Get for ArrayMeasurement. Use it to feed a pointer of any subclass of
   * vtkAbstractArrayMeasurement,
   * which will be used to accumulate scalars from the input data set in order to produce the hyper
   * tree grid.
   */
  vtkGetMacro(ArrayMeasurement, vtkAbstractArrayMeasurement*);
  vtkSetMacro(ArrayMeasurement, vtkAbstractArrayMeasurement*);
  //@}

  //@{
  /**
   * Set/Get for ArrayMeasurement. Use it to feed a pointer of any subclass of
   * vtkAbstractArrayMeasurement,
   * which will be used to accumulate scalars from the input data. This measurement is not used for
   * subdivision,
   * it is only used for display purposes.
   * It should be used when subdividing using vtkStandardDeviationArrayMeasurement or
   * vtkEntropyArrayMeasurement
   * for example.
   * Default value is nullptr.
   */
  vtkGetMacro(ArrayMeasurementDisplay, vtkAbstractArrayMeasurement*);
  vtkSetMacro(ArrayMeasurementDisplay, vtkAbstractArrayMeasurement*);
  //@}

  //@{
  /**
   * Set/Get the maximum tree depth.
   */
  vtkSetMacro(MaxDepth, unsigned int);
  vtkGetMacro(MaxDepth, unsigned int);
  //@}

  //@{
  /**
   * Get/Set the maximum grid size of the hyper tree grid. All further resolution is creaded by
   * hyper trees.
   * The default is [4, 4, 4].
   */
  vtkSetVector3Macro(Dimensions, unsigned int);
  vtkGetVector3Macro(Dimensions, unsigned int);
  //@}

  //@{
  /**
   * Upper bound used to decide whether hyper tree grid should be refined or not
   * Default value is std::numeric_limits<double>infinity().
   */
  vtkGetMacro(Max, double);
  vtkSetMacro(Max, double);
  //@}

  //@{
  /**
   * Sets Max to infinity. Call this method to annihilate the upper bound.
   */
  void SetMaxToInfinity();
  //@}

  //@{
  /**
   * Lower bound used to decide whether hyper tree grid should be refined or not
   * Default value is -std::numeric_limits<double>::infinity().
   */
  vtkGetMacro(Min, double);
  vtkSetMacro(Min, double);
  //@}

  //@{
  /**
   * Sets Min to minus infinity. Call this method to annihilate the lower bound.
   */
  void SetMinToInfinity();
  //@}

  //@{
  /**
   * Accessors for NoEmptyCells. If this flag is turned off, there can be masked leaves
   * although the input has geometry in them, just because there is no point inside the leaf.
   * If turned on, any empty leaf that has geometry will be display, i.e. the concerned leaves
   * parent
   * won't subdivide and create them, resuling in a "hole free" hyper tree grid, at the cost of a
   * coarser
   * result.
   */
  vtkGetMacro(NoEmptyCells, bool);
  vtkSetMacro(NoEmptyCells, bool);
  //@}

  //@{
  /**
   * Accessor for InRange. If set to true, the criterion for subdividing is true if the value is
   * within
   * the range. Otherwise, we subdivide for values outside of the range.
   */
  vtkGetMacro(InRange, bool);
  vtkSetMacro(InRange, bool);
  //@}

  //@{
  /**
   * Accessor for MinimumNumberOfPointsInSubtree, which sets a minimum number of points per leaf.
   * Note that the minimum number of points per subtree can be greater depending of the minimum
   * number of points required vtkAbstractArrayMeasurement*
   */
  vtkGetMacro(MinimumNumberOfPointsInSubtree, vtkIdType);
  vtkSetMacro(MinimumNumberOfPointsInSubtree, vtkIdType);
  //@}

  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestDataObject(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  //@{
  /**
   * If state == false, sets Max / Min to infinity / -infinity. Else, sets Max / Min to the last non
   * infinite value.
   */
  void SetMaxState(bool state);
  void SetMinState(bool state);
  //@}

protected:
  vtkResampleToHyperTreeGrid();
  ~vtkResampleToHyperTreeGrid() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int RequestData(vtkInformation* request, vtkInformationVector**, vtkInformationVector*);

  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*);
  int GenerateTrees(vtkHyperTreeGrid*);

  unsigned int BranchFactor;
  unsigned int Dimensions[3];
  unsigned int MaxDepth;

  /**
   * Range values used to decide whether to subdivide a leaf or not
   */
  double Min, Max;

  /**
   * Decides whether the criteria should be inside [this->Min, this->Max] or outside.
   */
  bool InRange;

  double Progress;

  vtkBitArray* Mask;

  /**
   * Only needed internally. Each element for each position in the multi resolution grid
   * stores the number of points in the subtree, as well as the concatenation of all the
   * accumulators
   * in the subtree. The data to accumulate needs to be associative.
   */
  struct GridElement
  {
    GridElement()
      : NumberOfLeavesInSubtree(0)
      , NumberOfPointsInSubtree(0)
      , NumberOfNonMaskedChildren(0)
      , AccumulatedWeight(0.0)
      , UnmaskedChildrenHaveNoMaskedLeaves(true)
      , CanSubdivide(false)
    {
    }
    virtual ~GridElement();

    /**
     * Accumulators used for measuring quantities on subtrees
     */
    std::vector<vtkAbstractAccumulator*> Accumulators;

    vtkIdType NumberOfLeavesInSubtree;
    vtkIdType NumberOfPointsInSubtree;
    vtkIdType NumberOfNonMaskedChildren;
    double AccumulatedWeight;

    /**
     * Boolean used to avoid searching for masked children in a full subtree
     */
    bool UnmaskedChildrenHaveNoMaskedLeaves;

    /**
     * Flag to tell whether it is legit to subdivide the corresponding leaf in the hyper tree.
     * A leaf cannot be subdivided if there is not enough data in the induced leaves.
     */
    bool CanSubdivide;
  };

  /**
   * Only needed internally. This draws a multi resolution grid that should have this->MaxDepth+1
   * grids.
   * The resolution of each grid depends on the branch factor and the depth.
   * There should be, in a given direction, this->BranchFactor ^ depth elements.
   * Indexing is as follows:
   * elements indexed at (i,j,k) at depth maps to the element indexed at
   * (i/this->BranchFactor, j/this->BranchFactor, k/this->BranchFactor) at depth-1.
   * Element (i,j,k) bijectively maps to idx = this->CoordinatesToIndex(i,j,k,depth), and
   * idx bijectively maps to element (i,j,k) = this->IndexToCoordinates(idx,depth).
   * this->CoordinatesToIndex is the inverse function of this->IndexToCoordinates.
   */
  typedef std::vector<std::unordered_map<vtkIdType, GridElement> > MultiResGridType;

  /**
   * Method that divides recursively leafs of the output hyper tree grid depending of the
   * subdivision criterion
   * The cursor should correspond with the multiResolutionGrid and (i,j,k) should match the position
   * of the cursor in the hyper tree grid.
   */
  void SubdivideLeaves(vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId, vtkIdType i,
    vtkIdType j, vtkIdType k, MultiResGridType& multiResolutionGrid);

  /**
   * Given an input dataSet and its corresponding scalar field data, fills a grid of multi
   * resolution grids
   * matching the subdivision scheme of the output hyper tree grid.
   */
  void CreateGridOfMultiResolutionGrids(
    vtkDataSet* dataSet, vtkDataArray* data, int fieldAssociation);

  /**
   * Helper for easy access to the cells dimensions of the hyper tree grid.
   */
  unsigned int CellDims[3];

  /**
   * Stores the number of children given input dimensions and branch factor
   */
  vtkIdType NumberOfChildren;

  /**
   * Output scalar/vector field
   */
  vtkDoubleArray *ScalarField, *DisplayScalarField;
  vtkLongArray *NumberOfLeavesInSubtreeField, *NumberOfPointsInSubtreeField;

  /**
   * Minimum number of points in a leaf for it to be subdivided.
   */
  vtkIdType MinimumNumberOfPointsInSubtree;

  /**
   * Helper to the maximum resolution in one direction in a hyper tree.
   */
  vtkIdType MaxResolutionPerTree;

  /**
   * Helper to the resolution at a certain depth in one direction of a hyper tree.
   */
  std::vector<vtkIdType> ResolutionPerTree;
  std::vector<double> Diagonal;

  /**
   * Dummy pointer for creating at run-time the proper type of ArrayMeasurement or
   * ArrayMeasurementDisplay.
   */
  vtkAbstractArrayMeasurement* ArrayMeasurement;
  vtkAbstractArrayMeasurement* ArrayMeasurementDisplay;

  /**
   * Converts indexing at given resolution to a tuple (i,j,k) to navigate in a MultiResolutionGrid.
   */
  vtkTuple<vtkIdType, 3> IndexToMultiResGridCoordinates(vtkIdType idx, std::size_t depth) const;

  /**
   * Converts indexing to coordinates for this->GridOfMultiResolutionGrids.
   */
  vtkTuple<vtkIdType, 3> IndexToGridCoordinates(std::size_t idx) const;

  /**
   * Converts (i,j,k) to the corresponding index at a certain depth to navigate in a
   * MultiResolutionGrid.
   */
  vtkIdType MultiResGridCoordinatesToIndex(
    vtkIdType i, vtkIdType j, vtkIdType k, std::size_t depth) const;

  /**
   * Converts (i,j,k) to the corresponding MultiResGrid location in
   * this->GridOfMultiResolutionGrids.
   */
  std::size_t GridCoordinatesToIndex(vtkIdType i, vtkIdType j, vtkIdType k) const;

  //@{
  /**
   * 3D grid of multi-resolution grids.
   * this->GridOfMultiResolutionGrids[this->GridCoordinatesToIndex(i,j,k)][depth][idx] is a
   * GridElement
   * in the MultiResGrid at coordinates (i,j,k).
   */
  typedef std::vector<MultiResGridType> GridOfMultiResGridsType;
  GridOfMultiResGridsType GridOfMultiResolutionGrids;
  //@}

  /**
   * Maps from the vector of vtkAbstractAccumulator* of GridElement to their position
   * in the accumulator needed by either this->ArrayMeasurementDisplay.
   */
  std::vector<std::size_t> ArrayMeasurementDisplayAccumulatorMap;

  /**
   * Accumulators needed to compute the measures. They are used as dummy pointers
   * to create the correct instances when creating the grid of multi-resolution grids.
   */
  std::vector<vtkAbstractAccumulator*> Accumulators;

  //@{
  /**
   * Buffer to store the pointers of the correct accumulator when measuring.
   * We only use them if this->ArrayMeasurementDisplay != nullptr
   */
  std::vector<vtkAbstractAccumulator*> ArrayMeasurementAccumulators;
  std::vector<vtkAbstractAccumulator*> ArrayMeasurementDisplayAccumulators;
  //@}

  /**
   * Method which will forbid subdividing cells if they have an empty children intersecting
   * a cell of the input.
   * inputs x, closestPoint, pcoords and weights are used to call cell->EvaluatePosition.
   * weights should be of size cell->GetNumberOfPoints().
   *
   * (i,j,k) are the hyper tree coordinates in the hyper tree grid. (ii,jj,kk) are the coordinates
   * of a cell
   * in an hypertree at depth.
   *
   * bounds are the input data set bounds (not the cell's bounds).
   */
  bool RecursivelyFillGaps(vtkCell* cell, const double bounds[6], const double cellBounds[6],
    vtkIdType i, vtkIdType j, vtkIdType k, double x[3], double closestPoint[3], double pcoords[3],
    double* weights, vtkIdType ii = 0, vtkIdType jj = 0, vtkIdType kk = 0, std::size_t depth = 0);

  /**
   * Cache used to handle SetMaxState(bool) and SetMinState(bool)
   */
  double MaxCache, MinCache;

  /**
   * Flag to say whether we accept empty cell with geometry in it or not.
   * Setting it on will make the filter slightly slower.
   */
  bool NoEmptyCells;

private:
  vtkResampleToHyperTreeGrid(vtkResampleToHyperTreeGrid&) = delete;
  void operator=(vtkResampleToHyperTreeGrid&) = delete;
};

#endif
