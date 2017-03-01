/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreeSliceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreeSliceFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkCutter.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPProbeFilter.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedIntArray.h"

#include <math.h>

vtkStandardNewMacro(vtkThreeSliceFilter);

//----------------------------------------------------------------------------
vtkThreeSliceFilter::vtkThreeSliceFilter()
{
  // Setup output ports [ Merge, SliceX, SliceY, SliceZ]
  this->SetNumberOfOutputPorts(4);

  // Setup Merge filter
  this->CombinedFilteredInput = vtkAppendPolyData::New();

  for (int i = 0; i < 3; ++i)
  {
    // Allocate internal vars
    this->Slices[i] = vtkCutter::New();
    this->Planes[i] = vtkPlane::New();
    this->Slices[i]->SetCutFunction(this->Planes[i]);

    // Bind pipeline
    this->CombinedFilteredInput->AddInputConnection(this->Slices[i]->GetOutputPort());
  }
  this->Probe = NULL;
  this->PointToProbe = vtkPointSource::New();
  this->PointToProbe->SetNumberOfPoints(1);
  this->PointToProbe->SetRadius(0);
  this->SetToDefaultSettings();
}

//----------------------------------------------------------------------------
vtkThreeSliceFilter::~vtkThreeSliceFilter()
{
  for (int i = 0; i < 3; ++i)
  {
    this->Slices[i]->Delete();
    this->Slices[i] = NULL;
    this->Planes[i]->Delete();
    this->Planes[i] = NULL;
  }

  this->CombinedFilteredInput->Delete();
  this->CombinedFilteredInput = NULL;

  if (this->PointToProbe)
  {
    this->PointToProbe->Delete();
    this->PointToProbe = NULL;
  }
  if (this->Probe)
  {
    this->Probe->ReleaseDataFlagOn();
    this->Probe->Delete();
    this->Probe = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetToDefaultSettings()
{
  double origin[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; ++i)
  {
    // Setup normal
    double normal[3] = { 0, 0, 0 };
    normal[i] = 1.0;

    // Reset planes origin/normal
    this->Planes[i]->SetOrigin(origin);
    this->Planes[i]->SetNormal(normal);

    // Reset number of slice
    this->Slices[i]->SetNumberOfContours(0);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkThreeSliceFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType internalMTime = 0;

  // Test Append filter
  internalMTime = this->CombinedFilteredInput->GetMTime();
  mTime = (internalMTime > mTime ? internalMTime : mTime);

  // Test slices
  for (int i = 0; i < 3; ++i)
  {
    internalMTime = this->Slices[i]->GetMTime();
    mTime = (internalMTime > mTime ? internalMTime : mTime);
  }

  return mTime;
}
//----------------------------------------------------------------------------
void vtkThreeSliceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (int i = 0; i < 3; ++i)
  {
    os << indent << " - Plane[" << i << "]: normal(" << this->Planes[i]->GetNormal()[0] << ", "
       << this->Planes[i]->GetNormal()[1] << ", " << this->Planes[i]->GetNormal()[2]
       << ") - origin(" << this->Planes[i]->GetOrigin()[0] << ", "
       << this->Planes[i]->GetOrigin()[1] << ", " << this->Planes[i]->GetOrigin()[2] << ")\n";
  }
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetCutNormal(int cutIndex, double normal[3])
{
  this->Planes[cutIndex]->SetNormal(normal);
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetCutOrigin(int cutIndex, double origin[3])
{
  this->Planes[cutIndex]->SetOrigin(origin);
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetCutOrigins(double origin[3])
{
  this->Planes[0]->SetOrigin(origin);
  this->Planes[1]->SetOrigin(origin);
  this->Planes[2]->SetOrigin(origin);
  this->PointToProbe->SetCenter(origin);
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetCutValue(int cutIndex, int index, double value)
{
  vtkCutter* slice = this->Slices[cutIndex];
  if (slice->GetValue(index) != value)
  {
    slice->SetValue(index, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetNumberOfSlice(int cutIndex, int size)
{
  vtkCutter* slice = this->Slices[cutIndex];
  if (size != slice->GetNumberOfContours())
  {
    slice->SetNumberOfContours(size);
    this->Modified();
  }
}
//-----------------------------------------------------------------------
int vtkThreeSliceFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
//-----------------------------------------------------------------------
int vtkThreeSliceFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get input
  vtkDataObject* input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* inputDataset = vtkDataSet::SafeDownCast(input);
  assert(inputDataset);

  // get outputs
  vtkPolyData* outputs[4];
  for (int i = 0; i < 4; ++i)
  {
    outputs[i] = vtkPolyData::SafeDownCast(
      outputVector->GetInformationObject(i)->Get(vtkDataObject::DATA_OBJECT()));
  }

  this->Process(inputDataset, outputs, VTK_UNSIGNED_INT_MAX);

  // Probe input if needed
  if (this->Probe)
  {
    this->PointToProbe->Update();
    this->Probe->SetInputData(1, input);
    this->Probe->SetInputData(0, this->PointToProbe->GetOutput());
    this->Probe->Update();
  }

  return 1;
}

namespace
{
class CutWorker
{
public:
  CutWorker(vtkCutter* slices[3])
    : Slices(slices)
  {
  }
  void operator()(vtkIdType begin, vtkIdType end)
  {
    for (vtkIdType idx = begin; idx < end; ++idx)
    {
      Slices[idx]->Update();
    }
  }

private:
  vtkCutter** Slices;
};
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::Process(
  vtkDataSet* input, vtkPolyData* outputs[4], unsigned int compositeIndex)
{
  // Process dataset
  // Add CellIds to allow cell selection to work
  vtkIdType nbCells = input->GetNumberOfCells();
  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetName("vtkSliceOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetNumberOfTuples(nbCells);
  input->GetCellData()->AddArray(originalCellIds.GetPointer());

  // Fill the array with proper id values
  for (vtkIdType id = 0; id < nbCells; ++id)
  {
    originalCellIds->SetValue(id, id);
  }

  // Add composite index information if we have any
  if (compositeIndex != VTK_UNSIGNED_INT_MAX)
  {
    vtkNew<vtkUnsignedIntArray> compositeIndexArray;
    compositeIndexArray->SetName("vtkSliceCompositeIndex");
    compositeIndexArray->SetNumberOfComponents(1);
    compositeIndexArray->SetNumberOfTuples(nbCells);
    compositeIndexArray->FillComponent(0, compositeIndex);
    input->GetCellData()->AddArray(compositeIndexArray.GetPointer());
  }

  // Setup internal pipeline
  this->Slices[0]->SetInputData(input);
  this->Slices[1]->SetInputData(input);
  this->Slices[2]->SetInputData(input);

  // Update components in parallel
  CutWorker worker(this->Slices);
  vtkSMPTools::For(0, 3, 1, worker);

  // Update the internal pipeline
  this->CombinedFilteredInput->Update();

  // Copy generated output to filter output
  outputs[0]->ShallowCopy(this->CombinedFilteredInput->GetOutput());
  outputs[1]->ShallowCopy(this->Slices[0]->GetOutput());
  outputs[2]->ShallowCopy(this->Slices[1]->GetOutput());
  outputs[3]->ShallowCopy(this->Slices[2]->GetOutput());

  // Add meta-data for sliced data
  vtkNew<vtkFloatArray> sliceAtOrigin;
  sliceAtOrigin->SetName("SliceAt");
  sliceAtOrigin->SetNumberOfComponents(3);
  sliceAtOrigin->SetNumberOfTuples(1);
  sliceAtOrigin->SetTuple(0, this->PointToProbe->GetCenter());
  for (int i = 0; i < 3; ++i)
  {
    vtkNew<vtkIdTypeArray> sliceIndex;
    sliceIndex->SetName("SliceAlongAxis");
    sliceIndex->SetNumberOfComponents(1);
    sliceIndex->SetNumberOfTuples(1);
    sliceIndex->SetTuple1(0, i);
    outputs[i + 1]->GetFieldData()->AddArray(sliceAtOrigin.GetPointer());
    outputs[i + 1]->GetFieldData()->AddArray(sliceIndex.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::EnableProbe(int enable)
{
  if (enable != 0 && this->Probe == NULL)
  {
    this->Probe = vtkPProbeFilter::New();
  }
  else if (this->Probe != NULL)
  {
    this->Probe->Delete();
    this->Probe = NULL;
  }
}
//----------------------------------------------------------------------------
bool vtkThreeSliceFilter::GetProbedPointData(const char* arrayName, double& value)
{
  if (this->Probe && this->Probe->GetOutput() && this->Probe->GetOutput()->GetPointData())
  {
    vtkDataArray* array = this->Probe->GetOutput()->GetPointData()->GetArray(arrayName);
    vtkDataArray* mask = this->Probe->GetOutput()->GetPointData()->GetArray("vtkValidPointMask");
    if (array && array->GetNumberOfComponents() == 1 && array->GetNumberOfTuples() > 0)
    {
      bool valid = false;
      value = array->GetVariantValue(0).ToDouble(&valid);
      if (valid && mask)
      {
        return (array->GetVariantValue(0).ToUnsignedChar() != 0);
      }
      return valid;
    }
  }
  return false;
}
