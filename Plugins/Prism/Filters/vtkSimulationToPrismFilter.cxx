/*=========================================================================

  Program:   ParaView
  Module:    vtkSimulationToPrismFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimulationToPrismFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSimulationToPrismFilter);

//------------------------------------------------------------------------------
vtkSimulationToPrismFilter::vtkSimulationToPrismFilter()
{
  this->XArrayName = nullptr;
  this->YArrayName = nullptr;
  this->ZArrayName = nullptr;
  this->AttributeType = vtkDataObject::CELL;
}

//------------------------------------------------------------------------------
vtkSimulationToPrismFilter::~vtkSimulationToPrismFilter()
{
  this->SetXArrayName(nullptr);
  this->SetYArrayName(nullptr);
  this->SetZArrayName(nullptr);
}

//------------------------------------------------------------------------------
void vtkSimulationToPrismFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XArrayName: " << (this->XArrayName ? this->XArrayName : "(nullptr)") << endl;
  os << indent << "YArrayName: " << (this->YArrayName ? this->YArrayName : "(nullptr)") << endl;
  os << indent << "ZArrayName: " << (this->ZArrayName ? this->ZArrayName : "(nullptr)") << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}

//------------------------------------------------------------------------------
int vtkSimulationToPrismFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

namespace
{

//------------------------------------------------------------------------------
struct InterpolatePointsFunctor
{
  vtkDataSet* Input;
  vtkPointData* InputPD;
  vtkPointData* OutputPD;
  int MaxCellSize;

  struct LocalData
  {
    std::vector<double> Weights;
    vtkSmartPointer<vtkIdList> CellPointIds;
  };

  vtkSMPThreadLocal<LocalData> TLData;

  InterpolatePointsFunctor(vtkDataSet* input, vtkPointData* inputPD, vtkPointData* outputPD)
    : Input(input)
    , InputPD(inputPD)
    , OutputPD(outputPD)
    , MaxCellSize(input->GetMaxCellSize())
  {
    if (auto polyData = vtkPolyData::SafeDownCast(input))
    {
      if (polyData->NeedToBuildCells())
      {
        polyData->BuildCells();
      }
    }
  }

  void Initialize()
  {
    auto& tlData = this->TLData.Local();
    tlData.Weights.resize(this->MaxCellSize);
    tlData.CellPointIds = vtkSmartPointer<vtkIdList>::New();
    tlData.CellPointIds->Allocate(this->MaxCellSize);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tlData = this->TLData.Local();
    auto& weights = tlData.Weights;
    auto& cellPointIds = tlData.CellPointIds;

    vtkIdType numPoints;
    for (vtkIdType i = begin; i < end; ++i)
    {
      this->Input->GetCellPoints(i, cellPointIds);
      numPoints = cellPointIds->GetNumberOfIds();
      if (numPoints > 0)
      {
        // set all weights with 1/numPoints
        std::fill(weights.begin(), weights.begin() + numPoints, 1.0 / numPoints);
        this->OutputPD->InterpolatePoint(this->InputPD, i, cellPointIds, weights.data());
      }
    }
  }

  void Reduce() {}
};

//------------------------------------------------------------------------------
template <typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ>
struct SimulationToPrismFunctor
{
  ArrayTypeX* ArrayX;
  ArrayTypeY* ArrayY;
  ArrayTypeZ* ArrayZ;
  vtkDoubleArray* Coordinates;

  SimulationToPrismFunctor(
    ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ, vtkDoubleArray* coordinates)
    : ArrayX(arrayX)
    , ArrayY(arrayY)
    , ArrayZ(arrayZ)
    , Coordinates(coordinates)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto inX = vtk::DataArrayValueRange<1>(this->ArrayX, begin, end).begin();
    auto inY = vtk::DataArrayValueRange<1>(this->ArrayY, begin, end).begin();
    auto inZ = vtk::DataArrayValueRange<1>(this->ArrayZ, begin, end).begin();
    auto outCoords = vtk::DataArrayTupleRange<3>(this->Coordinates, begin, end);

    for (auto coord : outCoords)
    {
      coord[0] = *inX++;
      coord[1] = *inY++;
      coord[2] = *inZ++;
    }
  }
};

//------------------------------------------------------------------------------
struct SimulationToPrismWorker
{
  template <typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ>
  void operator()(
    ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ, vtkDoubleArray* coordinates)
  {
    SimulationToPrismFunctor<ArrayTypeX, ArrayTypeY, ArrayTypeZ> functor(
      arrayX, arrayY, arrayZ, coordinates);
    vtkSMPTools::For(0, coordinates->GetNumberOfTuples(), functor);
  }
};
}

//------------------------------------------------------------------------------
int vtkSimulationToPrismFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // check that array names are set
  if (this->XArrayName == nullptr || this->YArrayName == nullptr || this->ZArrayName == nullptr)
  {
    vtkErrorMacro(<< "No array names were set!");
    return 1;
  }

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(vtkDataObject::GetData(inputVector[0], 0));
  vtkPolyData* output = vtkPolyData::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  vtkFieldData* inFD = input->GetAttributesAsFieldData(this->AttributeType);

  // get the point-data arrays
  vtkDataArray* xFD = inFD->GetArray(this->XArrayName);
  vtkDataArray* yFD = inFD->GetArray(this->YArrayName);
  vtkDataArray* zFD = inFD->GetArray(this->ZArrayName);

  // check that the arrays were found
  if (xFD == nullptr || yFD == nullptr || zFD == nullptr)
  {
    if (xFD == nullptr)
    {
      vtkErrorMacro(<< "X array " << this->XArrayName << " not found!");
    }
    if (yFD == nullptr)
    {
      vtkErrorMacro(<< "Y array " << this->YArrayName << " not found!");
    }
    if (zFD == nullptr)
    {
      vtkErrorMacro(<< "Z array " << this->ZArrayName << " not found!");
    }
    return 1;
  }
  // check that all arrays are scalars
  if (xFD->GetNumberOfComponents() != 1 || yFD->GetNumberOfComponents() != 1 ||
    zFD->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(<< "The arrays must have scalar values!");
    return 1;
  }

  // create points
  vtkIdType numberOfPoints = xFD->GetNumberOfTuples();
  vtkNew<vtkDoubleArray> coordinates;
  coordinates->SetNumberOfComponents(3);
  coordinates->SetNumberOfTuples(numberOfPoints);

  SimulationToPrismWorker simulationToPrismWorker;
  using Dispatcher = vtkArrayDispatch::Dispatch3SameValueType;
  if (!Dispatcher::Execute(xFD, yFD, zFD, simulationToPrismWorker, coordinates.Get()))
  {
    simulationToPrismWorker(xFD, yFD, zFD, coordinates.Get());
  }

  // set points
  vtkNew<vtkPoints> points;
  points->SetData(coordinates);
  output->SetPoints(points);

  // interpolate point data
  vtkIdType numberOfCells = input->GetNumberOfCells();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, numberOfCells);
  outPD->SetNumberOfTuples(numberOfCells);
  InterpolatePointsFunctor interpolatePointsFunctor(
    input, input->GetPointData(), output->GetPointData());
  vtkSMPTools::For(0, numberOfCells, interpolatePointsFunctor);

  // create cell array
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(numberOfCells);
  vtkSMPTools::For(0, numberOfCells, [&](vtkIdType begin, vtkIdType end) {
    for (vtkIdType i = begin; i < end; i++)
    {
      connectivity->SetValue(i, i);
    }
  });
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(numberOfCells + 1);
  vtkSMPTools::For(0, numberOfCells + 1, [&](vtkIdType begin, vtkIdType end) {
    for (vtkIdType i = begin; i < end; i++)
    {
      offsets->SetValue(i, i);
    }
  });

  // set cell array
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(offsets, connectivity);
  output->SetVerts(cellArray);

  // copy cell data
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->PassData(inCD);

  return 1;
}
