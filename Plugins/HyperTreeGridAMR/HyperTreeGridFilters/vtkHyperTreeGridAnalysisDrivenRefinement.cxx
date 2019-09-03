/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAnalysisDrivenRefinement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHyperTreeGridAnalysisDrivenRefinement.h"

#include "vtkAbstractArrayMeasurement.h"
#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTuple.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>

vtkStandardNewMacro(vtkHyperTreeGridAnalysisDrivenRefinement);

//----------------------------------------------------------------------------
vtkHyperTreeGridAnalysisDrivenRefinement::vtkHyperTreeGridAnalysisDrivenRefinement()
{
  // This a source: no input ports
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->BranchFactor = 2;
  this->MaxDepth = 1;
  this->ArrayMeasurement = nullptr;
  this->ArrayMeasurementDisplay = nullptr;
  this->DisplayScalarField = nullptr;
  this->Min = -std::numeric_limits<double>::infinity();
  this->Max = std::numeric_limits<double>::infinity();
  this->MinimumNumberOfPointsInSubtree = 1;
  this->InRange = true;
}

//----------------------------------------------------------------------------
vtkHyperTreeGridAnalysisDrivenRefinement::~vtkHyperTreeGridAnalysisDrivenRefinement() = default;

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAnalysisDrivenRefinement::FillInputPortInformation(int, vtkInformation* info)
{
  // This filter uses the vtkDataSet cell traversal methods so it
  // suppors any data set type as input.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

int vtkHyperTreeGridAnalysisDrivenRefinement::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

int vtkHyperTreeGridAnalysisDrivenRefinement::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the information objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hypertrees
  // because it is not generated yet and this process depends on the recursion formula.
  // Just send an upper limit instead.
  outInfo->Set(vtkHyperTreeGrid::LEVELS(), this->MaxDepth);
  outInfo->Set(vtkHyperTreeGrid::DIMENSION(), 3);
  outInfo->Set(vtkHyperTreeGrid::ORIENTATION(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAnalysisDrivenRefinement::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->UpdateProgress(0.0);

  // Get input and output data.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Skip execution if there is no input geometry.
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
  {
    vtkDebugMacro("No data to convert!");
    return 1;
  }

  output->Initialize();
  output->SetBranchFactor(this->BranchFactor);

  // Setting up a few useful values during the pipeline
  this->ResolutionPerTree.resize(this->MaxDepth);

  assert(this->MaxDepth && "Maximum depth has to be greater than one");
  this->ResolutionPerTree[0] = 1;

  for (std::size_t depth = 1; depth < this->ResolutionPerTree.size(); ++depth)
  {
    ResolutionPerTree[depth] = ResolutionPerTree[depth - 1] * this->BranchFactor;
  }
  this->MaxResolutionPerTree = this->ResolutionPerTree[this->MaxDepth - 1];

  // ensure that number of grid is a power of 2
  {
    int power = 0;
    for (size_t i = 0; i < 3; ++i)
    {
      power = log(this->Dimensions[i] - 1) / log(2);
      this->Dimensions[i] = pow(2, power) + 1;
    }
  }

  double* bounds = input->GetBounds();

  // Setting the point locations for the hyper tree grid
  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[0]);
    double step =
      this->Dimensions[0] > 1 ? (bounds[1] - bounds[0]) / (this->Dimensions[0] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[0]; ++i)
    {
      coords->SetValue(i, bounds[0] + step * i);
    }
    output->SetXCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[1]);
    double step =
      this->Dimensions[1] > 1 ? (bounds[3] - bounds[2]) / (this->Dimensions[1] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[1]; ++i)
    {
      coords->SetValue(i, bounds[2] + step * i);
    }
    output->SetYCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[2]);
    double step =
      this->Dimensions[2] > 1 ? (bounds[5] - bounds[4]) / (this->Dimensions[2] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[2]; ++i)
    {
      coords->SetValue(i, bounds[4] + step * i);
    }
    output->SetZCoordinates(coords);
  }

  output->SetDimensions(this->Dimensions);

  this->Mask = vtkBitArray::New();

  // Linking input scalar fields
  vtkSmartPointer<vtkDataArray> data = this->GetInputArrayToProcess(0, inputVector);
  vtkNew<vtkDoubleArray> scalarField;
  scalarField->SetName((std::string(data->GetName()) + std::string("_measure")).c_str());
  output->GetPointData()->AddArray(scalarField);
  this->ScalarField = scalarField;

  if (this->ArrayMeasurementDisplay)
  {
    vtkNew<vtkDoubleArray> scalarFieldDisplay;
    scalarFieldDisplay->SetName(data->GetName());
    output->GetPointData()->AddArray(scalarFieldDisplay);
    this->DisplayScalarField = scalarFieldDisplay;
  }

  vtkNew<vtkLongArray> numberOfLeavesInSubtreeField;
  numberOfLeavesInSubtreeField->SetName("Number of leaves");
  output->GetPointData()->AddArray(numberOfLeavesInSubtreeField);
  this->NumberOfLeavesInSubtreeField = numberOfLeavesInSubtreeField;

  vtkNew<vtkLongArray> numberOfPointsInSubtreeField;
  numberOfPointsInSubtreeField->SetName("Number of points");
  output->GetPointData()->AddArray(numberOfPointsInSubtreeField);
  this->NumberOfPointsInSubtreeField = numberOfPointsInSubtreeField;

  output->GetCellDims(this->CellDims);

  // Creating multi resolution grids used to construct the hyper tree grid
  // This multi resolution grid has the inner structure of the hyper tree grid
  // without its indexing. This is a bottom-up algorithm, which would be impossible
  // to process directly using a hyper tree grid because of its top-down structure.
  this->CreateGridOfMultiResolutionGrids(input, data);

  if (!this->GenerateTrees(output))
  {
    return 0;
  }

  // Cleaning our mess
  this->DeleteGridOfMultiResolutionGrids();

  output->SetMask(this->Mask);
  this->Mask->FastDelete();

  // Avoid keeping extra memory around.
  output->Squeeze();

  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::DeleteGridOfMultiResolutionGrids()
{
  for (std::size_t i = 0; i < this->GridOfMultiResolutionGrids.size(); ++i)
  {
    for (std::size_t j = 0; j < this->GridOfMultiResolutionGrids[i].size(); ++j)
    {
      for (std::size_t k = 0; k < this->GridOfMultiResolutionGrids[i][j].size(); ++k)
      {
        for (std::size_t depth = 0; depth < this->GridOfMultiResolutionGrids[i][j][k].size();
             ++depth)
        {
          for (auto& mapElement : this->GridOfMultiResolutionGrids[i][j][k][depth])
          {
            if (mapElement.second.ArrayMeasurement)
            {
              mapElement.second.ArrayMeasurement->FastDelete();
            }
            if (mapElement.second.ArrayMeasurementDisplay)
            {
              mapElement.second.ArrayMeasurementDisplay->FastDelete();
            }
          }
        }
      }
    }
  }
  this->GridOfMultiResolutionGrids.clear();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::CreateGridOfMultiResolutionGrids(
  vtkDataSet* dataSet, vtkDataArray* data)
{
  double* bounds = dataSet->GetBounds();

  // Creating the grid of multi resolution grids
  this->GridOfMultiResolutionGrids.resize(this->CellDims[0]);
  for (std::size_t i = 0; i < this->CellDims[0]; ++i)
  {
    this->GridOfMultiResolutionGrids[i].resize(this->CellDims[1]);
    for (std::size_t j = 0; j < this->CellDims[1]; ++j)
    {
      this->GridOfMultiResolutionGrids[i][j].resize(this->CellDims[2]);
      for (std::size_t k = 0; k < this->CellDims[2]; ++k)
      {
        this->GridOfMultiResolutionGrids[i][j][k].resize(this->MaxDepth);
      }
    }
  }

  // First pass, we fill the highest resolution grid with input values
  for (vtkIdType pointId = 0; pointId < dataSet->GetNumberOfPoints(); ++pointId)
  {
    double* point = dataSet->GetPoint(pointId);

    // (i, j, k) are the coordinates of the corresponding hyper tree
    unsigned int i = std::min(
                   static_cast<vtkIdType>((point[0] - bounds[0]) / (bounds[1] - bounds[0]) *
                     this->CellDims[0] * this->MaxResolutionPerTree),
                   this->CellDims[0] * this->MaxResolutionPerTree - 1),
                 j = std::min(
                   static_cast<vtkIdType>((point[1] - bounds[2]) / (bounds[3] - bounds[2]) *
                     this->CellDims[1] * this->MaxResolutionPerTree),
                   this->CellDims[1] * this->MaxResolutionPerTree - 1),
                 k = std::min(
                   static_cast<vtkIdType>((point[2] - bounds[4]) / (bounds[5] - bounds[4]) *
                     this->CellDims[2] * this->MaxResolutionPerTree),
                   this->CellDims[2] * this->MaxResolutionPerTree - 1);

    // We bijectively convert the local coordinates within a hyper tree grid to an integer to pass
    // it
    // to the std::unordered_map at highest resolution
    vtkIdType idx = this->CoordinatesToIndex(i % this->MaxResolutionPerTree,
      j % this->MaxResolutionPerTree, k % this->MaxResolutionPerTree, this->MaxDepth - 1);

    auto& grid =
      this->GridOfMultiResolutionGrids[i / this->MaxResolutionPerTree]
                                      [j / this->MaxResolutionPerTree]
                                      [k / this->MaxResolutionPerTree][this->MaxDepth - 1];

    auto it = grid.find(idx);
    // if this is the first time we pass by this grid location, we create a new ArrayMeasurement
    // instance
    // NOTE: GridElement::CanSubdivide does not need to be set at the highest resolution
    if (it == grid.end())
    {
      GridElement& element = grid[idx];
      element.NumberOfLeavesInSubtree = 1;
      element.NumberOfPointsInSubtree = 1;
      element.ArrayMeasurement = this->ArrayMeasurement->NewInstance();
      element.ArrayMeasurement->Add(data->GetTuple(pointId));
      if (this->ArrayMeasurementDisplay)
      {
        element.ArrayMeasurementDisplay = this->ArrayMeasurementDisplay->NewInstance();
        element.ArrayMeasurementDisplay->Add(data->GetTuple(pointId));
      }
    }
    // if not, then the grid location is already created, just need to add the element into it
    else
    {
      it->second.ArrayMeasurement->Add(data->GetTuple(pointId));
      if (this->ArrayMeasurementDisplay)
      {
        it->second.ArrayMeasurementDisplay->Add(data->GetTuple(pointId));
      }
      ++(it->second.NumberOfPointsInSubtree);
    }
  }

  // Now, we fill the multi-resolution grid bottom-up
  for (std::size_t i = 0; i < this->GridOfMultiResolutionGrids.size(); ++i)
  {
    for (std::size_t j = 0; j < this->GridOfMultiResolutionGrids[i].size(); ++j)
    {
      for (std::size_t k = 0; k < this->GridOfMultiResolutionGrids[i][j].size(); ++k)
      {
        auto& multiResolutionGrid = this->GridOfMultiResolutionGrids[i][j][k];
        for (std::size_t depth = this->MaxDepth - 1; depth; --depth)
        {
          // The strategy is the following:
          // Given an iterator on the elements of the grid at resolution depth,
          // we propagate the accumulated values to the lower resolution depth-1
          // using correct indexing
          for (const auto& mapElement : multiResolutionGrid[depth])
          {
            vtkTuple<vtkIdType, 3> coord = this->IndexToCoordinates(mapElement.first, depth);
            coord[0] /= this->BranchFactor;
            coord[1] /= this->BranchFactor;
            coord[2] /= this->BranchFactor;
            vtkIdType idx = this->CoordinatesToIndex(coord[0], coord[1], coord[2], depth - 1);

            // Same as before: if the grid location is not created yet, we create it, if not,
            // we merge the corresponding accumulated values
            auto it = multiResolutionGrid[depth - 1].find(idx);
            if (it == multiResolutionGrid[depth - 1].end())
            {
              GridElement& element = multiResolutionGrid[depth - 1][idx];
              element.NumberOfLeavesInSubtree = mapElement.second.NumberOfLeavesInSubtree;
              element.NumberOfPointsInSubtree = mapElement.second.NumberOfPointsInSubtree;
              element.CanSubdivide = mapElement.second.ArrayMeasurement->CanMeasure() &&
                this->MinimumNumberOfPointsInSubtree <= mapElement.second.NumberOfPointsInSubtree &&
                (!this->ArrayMeasurementDisplay ||
                                       mapElement.second.ArrayMeasurementDisplay->CanMeasure());
              element.ArrayMeasurement = this->ArrayMeasurement->NewInstance();
              element.ArrayMeasurement->Add(mapElement.second.ArrayMeasurement);
              if (this->ArrayMeasurementDisplay)
              {
                element.ArrayMeasurementDisplay = this->ArrayMeasurementDisplay->NewInstance();
                element.ArrayMeasurementDisplay->Add(mapElement.second.ArrayMeasurementDisplay);
              }
            }
            else
            {
              it->second.NumberOfLeavesInSubtree += mapElement.second.NumberOfLeavesInSubtree;
              it->second.NumberOfPointsInSubtree += mapElement.second.NumberOfPointsInSubtree;
              it->second.CanSubdivide &= mapElement.second.ArrayMeasurement->CanMeasure() &&
                this->MinimumNumberOfPointsInSubtree <= mapElement.second.NumberOfPointsInSubtree &&
                (!this->ArrayMeasurementDisplay ||
                                           mapElement.second.ArrayMeasurementDisplay->CanMeasure());
              it->second.ArrayMeasurement->Add(mapElement.second.ArrayMeasurement);
              if (this->ArrayMeasurementDisplay)
              {
                it->second.ArrayMeasurementDisplay->Add(mapElement.second.ArrayMeasurementDisplay);
              }
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAnalysisDrivenRefinement::GenerateTrees(vtkHyperTreeGrid* htg)
{
  // Iterate over all hyper trees
  this->Progress = 0.;

  vtkIdType treeOffset = 0;

  for (vtkIdType i = 0; i < htg->GetCellDims()[0]; ++i)
  {
    for (vtkIdType j = 0; j < htg->GetCellDims()[1]; ++j)
    {
      for (vtkIdType k = 0; k < htg->GetCellDims()[2]; ++k)
      {
        vtkIdType treeId;
        htg->GetIndexFromLevelZeroCoordinates(treeId, i, j, k);
        // Build this tree:
        vtkHyperTreeGridNonOrientedCursor* cursor = htg->NewNonOrientedCursor(treeId, true);
        cursor->GetTree()->SetGlobalIndexStart(treeOffset);
        // We subdivide each tree starting at position (0,0,0) at coarsest level
        // We feed the corresponding multi resolution grid
        // Top-down algorithm
        this->SubdivideLeaves(cursor, treeId, 0, 0, 0, this->GridOfMultiResolutionGrids[i][j][k]);
        treeOffset += cursor->GetTree()->GetNumberOfVertices();
        cursor->FastDelete();
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::SubdivideLeaves(
  vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId, std::size_t i, std::size_t j,
  std::size_t k, MultiResolutionGridType& multiResolutionGrid)
{
  vtkIdType level = cursor->GetLevel();
  vtkIdType vertexId = cursor->GetVertexId();
  vtkHyperTree* tree = cursor->GetTree();
  vtkIdType idx = tree->GetGlobalIndexFromLocal(vertexId);

  auto it = multiResolutionGrid[level].find(this->CoordinatesToIndex(i, j, k, level));
  double value =
    it != multiResolutionGrid[level].end() ? it->second.ArrayMeasurement->Measure() : 0;

  if (this->ArrayMeasurementDisplay)
  {
    this->DisplayScalarField->InsertValue(idx,
      it != multiResolutionGrid[level].end() ? it->second.ArrayMeasurementDisplay->Measure() : 0);
  }
  this->ScalarField->InsertValue(idx, value);
  this->NumberOfLeavesInSubtreeField->InsertValue(
    idx, it != multiResolutionGrid[level].end() ? it->second.NumberOfLeavesInSubtree : 0);
  this->NumberOfPointsInSubtreeField->InsertValue(
    idx, it != multiResolutionGrid[level].end() ? it->second.NumberOfPointsInSubtree : 0);
  this->Mask->InsertValue(idx, it == multiResolutionGrid[level].end());

  if (cursor->IsLeaf())
  {
    // If we match the criterion, we subdivide
    // Also: if the subtrees have only one element, it is useless to subdivide, we already are at
    // the finest
    // possible resolution given input data
    if (level < this->MaxDepth && it != multiResolutionGrid[level].end() &&
      it->second.CanSubdivide && (this->InRange && value > this->Min && value < this->Max ||
                                   !this->InRange && !(value > this->Min && value < this->Max)))
    {
      cursor->SubdivideLeaf();
    }
    else
    {
      return;
    }
  }

  // We iterate in the neighborhood and zoom into higher resolution level
  std::size_t ii = 0, jj = 0, kk = 0;
  for (int childIdx = 0; childIdx < cursor->GetNumberOfChildren(); ++childIdx)
  {
    cursor->ToChild(childIdx);
    this->SubdivideLeaves(cursor, treeId, i * tree->GetBranchFactor() + ii,
      j * tree->GetBranchFactor() + jj, k * tree->GetBranchFactor() + kk, multiResolutionGrid);
    cursor->ToParent();

    if (++ii == tree->GetBranchFactor())
    {
      if (++jj == tree->GetBranchFactor())
      {
        ++kk;
        jj = 0;
      }
      ii = 0;
    }
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAnalysisDrivenRefinement::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkHyperTreeGridAnalysisDrivenRefinement::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAnalysisDrivenRefinement::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; ++i)
  {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j = 0; j < numInputConnections; ++j)
    {
      vtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::SetMaxToInfinity()
{
  this->SetMax(std::numeric_limits<double>::infinity());
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAnalysisDrivenRefinement::SetMinToInfinity()
{
  this->SetMin(-std::numeric_limits<double>::infinity());
}

//----------------------------------------------------------------------------
vtkTuple<vtkIdType, 3> vtkHyperTreeGridAnalysisDrivenRefinement::IndexToCoordinates(
  vtkIdType idx, std::size_t depth) const
{
  vtkTuple<vtkIdType, 3> coord;
  coord[0] = idx % (this->ResolutionPerTree[depth]);
  coord[1] = (idx / this->ResolutionPerTree[depth]) % this->ResolutionPerTree[depth];
  coord[2] = idx / (this->ResolutionPerTree[depth] * this->ResolutionPerTree[depth]);
  return coord;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridAnalysisDrivenRefinement::CoordinatesToIndex(
  std::size_t i, std::size_t j, std::size_t k, std::size_t depth) const
{
  return i + j * this->ResolutionPerTree[depth] +
    k * this->ResolutionPerTree[depth] * this->ResolutionPerTree[depth];
}
