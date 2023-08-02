// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVExtractHistogram2D.h"

// VTK includes
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkGradientFilter.h"
#include "vtkGraph.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkPVExtractHistogram2D);
vtkCxxSetObjectMacro(vtkPVExtractHistogram2D, Controller, vtkMultiProcessController);

//-------------------------------------------------------------------------------------------------
vtkPVExtractHistogram2D::vtkPVExtractHistogram2D()
{
  this->InitializeCache();
}

//-------------------------------------------------------------------------------------------------
vtkPVExtractHistogram2D::~vtkPVExtractHistogram2D()
{
  if (this->ComponentArrayCache[0])
  {
    if (!strcmp(this->ComponentArrayCache[0]->GetName(), "Magnitude"))
    {
      this->ComponentArrayCache[0]->UnRegister(this);
      this->ComponentArrayCache[0] = nullptr;
    }
  }
  if (this->ComponentArrayCache[1])
  {
    if (!strcmp(this->ComponentArrayCache[1]->GetName(), "GradientMag"))
    {
      this->ComponentArrayCache[1]->UnRegister(this);
      this->ComponentArrayCache[1] = nullptr;
    }
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfBins = [" << this->NumberOfBins[0] << ", " << this->NumberOfBins[1]
     << "]" << endl;
  os << indent << "UseGradientForYAxis = " << (this->GetUseGradientForYAxis() ? "True" : "False")
     << endl;
  os << indent << "UseCustomBinRanges0 = " << this->UseCustomBinRanges0 << endl;
  os << indent << "CustomBinRanges0 = [" << this->CustomBinRanges0[0] << ", "
     << this->CustomBinRanges0[1] << "]" << endl;
  os << indent << "UseCustomBinRanges1 = " << this->UseCustomBinRanges1 << endl;
  os << indent << "CustomBinRanges1 = [" << this->CustomBinRanges1[0] << ", "
     << this->CustomBinRanges1[1] << "]" << endl;
}

//------------------------------------------------------------------------------------------------
int vtkPVExtractHistogram2D::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------------------------
int vtkPVExtractHistogram2D::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  this->InitializeCache();
  this->GetInputArrays(inputVector);
  this->ComputeComponentRange();

  int ext[6] = { 0, this->NumberOfBins[0] - 1, 0, this->NumberOfBins[1] - 1, 0, 0 };
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  double sp[3] = { this->OutputSpacing[0], this->OutputSpacing[1], 1.0 };
  double o[3] = { this->OutputOrigin[0], this->OutputOrigin[1], 0.0 };
  if (this->GetUseInputRangesForOutputBounds())
  {
    sp[0] =
      (this->ComponentRangeCache[0][1] - this->ComponentRangeCache[0][0]) / this->NumberOfBins[0];
    sp[1] =
      (this->ComponentRangeCache[1][1] - this->ComponentRangeCache[1][0]) / this->NumberOfBins[1];
    o[0] = this->ComponentRangeCache[0][0];
    o[1] = this->ComponentRangeCache[1][0];
  }
  outInfo->Set(vtkDataObject::SPACING(), sp, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), o, 3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::UNRESTRICTED_UPDATE_EXTENT(), 1);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);
  return 1;
}

//------------------------------------------------------------------------------------------------
int vtkPVExtractHistogram2D::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    int* ext = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
  }

  return 1;
}

//------------------------------------------------------------------------------------------------
int vtkPVExtractHistogram2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!input || !this->ComponentArrayCache[0])
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  double o[3] = { this->OutputOrigin[0], this->OutputOrigin[1], 0.0 };
  double sp[3] = { this->OutputSpacing[0], this->OutputSpacing[1], 1.0 };
  if (this->GetUseInputRangesForOutputBounds())
  {
    o[0] = this->ComponentRangeCache[0][0];
    o[1] = this->ComponentRangeCache[1][0];
    sp[0] =
      (this->ComponentRangeCache[0][1] - this->ComponentRangeCache[0][0]) / this->NumberOfBins[0];
    sp[1] =
      (this->ComponentRangeCache[1][1] - this->ComponentRangeCache[1][0]) / this->NumberOfBins[1];
  }
  output->SetDimensions(this->NumberOfBins[0], this->NumberOfBins[1], 1);
  output->SetOrigin(o);
  output->SetSpacing(sp);
  output->AllocateScalars(VTK_DOUBLE, 1);
  this->ComputeHistogram2D(output);

  return 1;
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::InitializeCache()
{
  this->ComponentIndexCache[0] = 0;
  this->ComponentIndexCache[1] = 0;
  this->ComponentArrayCache[0] = nullptr;
  this->ComponentArrayCache[1] = nullptr;
  this->GhostArray = nullptr;
  this->GhostsToSkip = 0;
  this->ComponentRangeCache[0][0] = 0.0;
  this->ComponentRangeCache[0][1] = 1.0;
  this->ComponentRangeCache[1][0] = 0.0;
  this->ComponentRangeCache[1][1] = 1.0;
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::ComputeVectorMagnitude(vtkDataArray* arr, vtkDataArray*& res)
{
  if (res)
  {
    res->UnRegister(this);
    res = nullptr;
  }

  int numComps = arr->GetNumberOfComponents();
  if (numComps == 1)
  {
    res = arr;
    return;
  }

  int numTuples = arr->GetNumberOfTuples();
  res = vtkDoubleArray::New();
  res->SetName("Magnitude");
  res->SetNumberOfComponents(1);
  res->SetNumberOfTuples(numTuples);

  auto arrRange = vtk::DataArrayTupleRange(arr);
  auto magRange = vtk::DataArrayTupleRange(res);

  for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
  {
    double sqSum = 0;
    for (vtk::ComponentIdType cId = 0; cId < numComps; ++cId)
    {
      double val = static_cast<double>(arrRange[tupleId][cId]);
      sqSum += val * val;
    }
    magRange[tupleId][0] = std::sqrt(sqSum);
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::GetInputArrays(vtkInformationVector** inputVector)
{
  if (!inputVector)
  {
    return;
  }

  int fieldAssociation;
  vtkDataArray* inputArray0 =
    vtkDataArray::SafeDownCast(this->GetInputArrayToProcess(0, inputVector, fieldAssociation));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);

  if (!inputArray0)
  {
    return;
  }
  if (this->Component0 == inputArray0->GetNumberOfComponents())
  {
    this->ComputeVectorMagnitude(inputArray0, this->ComponentArrayCache[0]);
  }
  else
  {
    this->ComponentArrayCache[0] = inputArray0;
  }

  vtkFieldData* dsa = inputDS
    ? inputDS->GetAttributesAsFieldData(this->GetInputArrayAssociation(0, inputVector))
    : nullptr;

  if (dsa)
  {
    this->GhostArray = dsa->GetGhostArray();
    this->GhostsToSkip = dsa->GetGhostsToSkip();
  }

  // Figure out if we are using the gradient magnitude for the Y axis
  if (this->UseGradientForYAxis)
  {
    if (this->ComponentArrayCache[0] != inputArray0)
    {
      // Vector Magnitude
      // Add the array to the input
      if (!inputDS)
      {
        vtkErrorMacro("Using the magnitude component of a vector array with gradient for Y axis "
                      "is only supported for vtkDataSet and subclasses.");
        return;
      }
      dsa->AddArray(this->ComponentArrayCache[0]);
    }
    this->ComputeGradient(input);

    if (this->ComponentArrayCache[0] != inputArray0)
    {
      dsa->RemoveArray("Magnitude");
    }
  }
  else
  {
    if (this->ComponentArrayCache[1])
    {
      if (!strcmp(this->ComponentArrayCache[1]->GetName(), "GradientMag"))
      {
        this->ComponentArrayCache[1]->UnRegister(this);
        this->ComponentArrayCache[1] = nullptr;
      }
    }
    vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
    if (inArrayVec->GetNumberOfInformationObjects() > 1)
    {
      this->ComponentArrayCache[1] = this->GetInputArrayToProcess(1, inputVector);
    }
    else
    {
      this->ComponentArrayCache[1] = this->ComponentArrayCache[0];
    }
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::ComputeComponentRange()
{
  if (!this->ComponentArrayCache[0] || !this->ComponentArrayCache[1])
  {
    return;
  }

  this->ComponentIndexCache[0] = this->Component0;
  if (this->ComponentIndexCache[0] < 0 ||
    this->ComponentIndexCache[0] > this->ComponentArrayCache[0]->GetNumberOfComponents())
  {
    this->ComponentIndexCache[0] = 0;
  }
  this->ComponentIndexCache[1] = this->Component1;
  if (this->ComponentIndexCache[1] < 0 ||
    this->ComponentIndexCache[1] > this->ComponentArrayCache[1]->GetNumberOfComponents())
  {
    this->ComponentIndexCache[1] = 0;
  }

  if (!this->UseCustomBinRanges0)
  {
    this->ComponentArrayCache[0]->GetFiniteRange(
      this->ComponentRangeCache[0], this->ComponentIndexCache[0]);
  }
  else
  {
    this->ComponentRangeCache[0][0] = this->CustomBinRanges0[0];
    this->ComponentRangeCache[0][1] = this->CustomBinRanges0[1];
  }
  if (this->ComponentRangeCache[0][1] < this->ComponentRangeCache[0][0])
  {
    double tmp = this->ComponentRangeCache[0][1];
    this->ComponentRangeCache[0][1] = this->ComponentRangeCache[0][0];
    this->ComponentRangeCache[0][0] = tmp;
  }

  if (this->UseGradientForYAxis)
  {
    this->ComponentArrayCache[1]->GetFiniteRange(
      this->ComponentRangeCache[1], this->ComponentIndexCache[1]);
  }
  else
  {
    if (this->UseCustomBinRanges1)
    {
      this->ComponentRangeCache[1][0] = this->CustomBinRanges1[0];
      this->ComponentRangeCache[1][1] = this->CustomBinRanges1[1];
    }
    else
    {
      this->ComponentArrayCache[1]->GetFiniteRange(
        this->ComponentRangeCache[1], this->ComponentIndexCache[1]);
    }
  }

  if (this->ComponentRangeCache[1][1] < this->ComponentRangeCache[1][0])
  {
    double tmp = this->ComponentRangeCache[1][1];
    this->ComponentRangeCache[1][1] = this->ComponentRangeCache[1][0];
    this->ComponentRangeCache[1][0] = tmp;
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::ComputeHistogram2D(vtkImageData* histogram)
{
  if (!this->ComponentArrayCache[0] || !histogram)
  {
    return;
  }

  auto histArray = histogram->GetPointData()->GetScalars();
  histArray->FillComponent(0, 0);

  const auto arr1Range = vtk::DataArrayTupleRange(this->ComponentArrayCache[0]);
  const auto arr2Range = vtk::DataArrayTupleRange(this->ComponentArrayCache[1]);
  auto histRange = vtk::DataArrayValueRange(histArray);

  const vtk::TupleIdType numTuples = arr1Range.size();
  if (arr2Range.size() != numTuples)
  {
    vtkErrorMacro("<< Both arrays should be the same size");
    return;
  }

  for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
  {
    const auto a1 = arr1Range[tupleId][this->ComponentIndexCache[0]];
    vtkIdType bin1 =
      static_cast<vtkIdType>((a1 - this->ComponentRangeCache[0][0]) * (this->NumberOfBins[0] - 1) /
        (this->ComponentRangeCache[0][1] - this->ComponentRangeCache[0][0]));
    bin1 = bin1 >= this->NumberOfBins[0] ? this->NumberOfBins[0] - 1 : bin1;
    const auto a2 = arr2Range[tupleId][this->ComponentIndexCache[1]];
    vtkIdType bin2 =
      static_cast<vtkIdType>((a2 - this->ComponentRangeCache[1][0]) * (this->NumberOfBins[1] - 1) /
        (this->ComponentRangeCache[1][1] - this->ComponentRangeCache[1][0]));
    bin2 = bin2 >= this->NumberOfBins[1] ? this->NumberOfBins[1] - 1 : bin2;
    vtkIdType histIndex = bin2 * this->NumberOfBins[0] + bin1;

    if (!this->GhostArray || !(this->GhostArray->GetValue(tupleId) & this->GhostsToSkip))
    {
      histRange[histIndex]++;
    }
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVExtractHistogram2D::ComputeGradient(vtkDataObject* input)
{
  if (!input)
  {
    return;
  }

  vtkNew<vtkGradientFilter> gf;
  gf->SetInputData(input);
  gf->SetComputeGradient(true);
  gf->SetComputeVorticity(false);
  gf->SetComputeDivergence(false);
  gf->SetComputeQCriterion(false);
  gf->SetResultArrayName("Gradient");
  gf->SetInputArrayToProcess(
    0, 0, 0, this->GetInputArrayAssociation(0, input), this->ComponentArrayCache[0]->GetName());
  gf->Update();

  const auto gradientArray = gf->GetOutput()
                               ->GetAttributesAsFieldData(this->GetInputArrayAssociation(0, input))
                               ->GetArray("Gradient");
  const auto gradientArrRange = vtk::DataArrayTupleRange(gradientArray);
  const vtk::TupleIdType numTuples = gradientArrRange.size();

  if (this->ComponentArrayCache[1])
  {
    if (!strcmp(this->ComponentArrayCache[1]->GetName(), "GradientMag"))
    {
      this->ComponentArrayCache[1]->UnRegister(this);
      this->ComponentArrayCache[1] = nullptr;
    }
  }

  this->ComponentArrayCache[1] = vtkDoubleArray::New();
  this->ComponentArrayCache[1]->SetName("GradientMag");
  this->ComponentArrayCache[1]->SetNumberOfComponents(1);
  this->ComponentArrayCache[1]->SetNumberOfTuples(numTuples);

  auto gradMagRange = vtk::DataArrayTupleRange(this->ComponentArrayCache[1]);

  // For each component, the gradient is a (3 component) tuple i.e. for input array with 3
  // components, the gradient array will have 9 components.
  // Compute the magnitude accounting for the component and accommodate the magnitude of the vector.
  int comp0 = this->Component0;
  if (comp0 == this->ComponentArrayCache[0]->GetNumberOfComponents())
  {
    comp0 = 0;
  }

  for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
  {
    double sqSum = 0.0;
    for (vtk::ComponentIdType cId = comp0 * 3; cId < comp0 * 3 + 3; ++cId)
    {
      double val = static_cast<double>(gradientArrRange[tupleId][cId]);
      val = val < 1e-5 ? 0.0 : val;
      sqSum += val * val;
    }
    gradMagRange[tupleId][0] = std::sqrt(sqSum);
  }
}
