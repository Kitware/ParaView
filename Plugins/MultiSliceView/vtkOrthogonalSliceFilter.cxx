/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrthogonalSliceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrthogonalSliceFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkContourValues.h"
#include "vtkCutter.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"

#include <math.h>

vtkStandardNewMacro(vtkOrthogonalSliceFilter);

//----------------------------------------------------------------------------
vtkOrthogonalSliceFilter::vtkOrthogonalSliceFilter()
{
  this->SliceAlongX = vtkCutter::New();
  this->SliceAlongY = vtkCutter::New();
  this->SliceAlongZ = vtkCutter::New();

  vtkNew<vtkPlane> planeX;
  planeX->SetNormal(1,0,0);
  planeX->SetOrigin(0,0,0);
  this->SliceAlongX->SetCutFunction(planeX.GetPointer());

  vtkNew<vtkPlane> planeY;
  planeY->SetNormal(0,1,0);
  planeY->SetOrigin(0,0,0);
  this->SliceAlongY->SetCutFunction(planeY.GetPointer());

  vtkNew<vtkPlane> planeZ;
  planeZ->SetNormal(0,0,1);
  planeZ->SetOrigin(0,0,0);
  this->SliceAlongZ->SetCutFunction(planeZ.GetPointer());

  this->CombinedFilteredInput = vtkAppendPolyData::New();
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongX->GetOutputPort());
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongY->GetOutputPort());
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongZ->GetOutputPort());
}

//----------------------------------------------------------------------------
vtkOrthogonalSliceFilter::~vtkOrthogonalSliceFilter()
{
  this->SliceAlongX->Delete();
  this->SliceAlongY->Delete();
  this->SliceAlongZ->Delete();
  this->SliceAlongX = this->SliceAlongY = this->SliceAlongZ = NULL;

  this->CombinedFilteredInput->Delete();
  this->CombinedFilteredInput = NULL;
}

//----------------------------------------------------------------------------
unsigned long vtkOrthogonalSliceFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long internalMTime = 0;

  // Test slice along X
  internalMTime = this->SliceAlongX->GetMTime();
  mTime = ( internalMTime > mTime ? internalMTime : mTime );

  // Test slice along Y
  internalMTime = this->SliceAlongY->GetMTime();
  mTime = ( internalMTime > mTime ? internalMTime : mTime );

  // Test slice along Z
  internalMTime = this->SliceAlongZ->GetMTime();
  mTime = ( internalMTime > mTime ? internalMTime : mTime );

  // Test Append filter
  internalMTime = this->CombinedFilteredInput->GetMTime();
  mTime = ( internalMTime > mTime ? internalMTime : mTime );

  return mTime;
}
//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetSliceX(int index, double value)
{
  this->SetSlice(this->SliceAlongX, index, value);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetNumberOfSliceX(int size)
{
  this->SetNumberOfSlice(this->SliceAlongX, size);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetSliceY(int index, double value)
{
  this->SetSlice(this->SliceAlongY, index, value);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetNumberOfSliceY(int size)
{
  this->SetNumberOfSlice(this->SliceAlongY, size);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetSliceZ(int index, double value)
{
  this->SetSlice(this->SliceAlongZ, index, value);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetNumberOfSliceZ(int size)
{
  this->SetNumberOfSlice(this->SliceAlongZ, size);
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetSlice(vtkCutter* slice, int index, double value)
{
  if(slice->GetValue(index) != value)
    {
    slice->SetValue(index, value);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkOrthogonalSliceFilter::SetNumberOfSlice(vtkCutter* slice, int size)
{
  if(size != slice->GetNumberOfContours())
    {
    slice->SetNumberOfContours(size);
    this->Modified();
    }
}
//-----------------------------------------------------------------------
int vtkOrthogonalSliceFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
//-----------------------------------------------------------------------
int vtkOrthogonalSliceFilter::RequestData( vtkInformation *,
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output =
      vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Add CellIds
  vtkIdType nbCells = input->GetNumberOfCells();
  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetName("vtkSliceOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetNumberOfTuples(nbCells);
  input->GetCellData()->AddArray(originalCellIds.GetPointer());

  // Fill the array with proper id values
  for(vtkIdType id = 0; id < nbCells; ++id)
    {
    originalCellIds->SetValue(id, id);
    }

  this->SliceAlongX->SetInputData(input);
  this->SliceAlongY->SetInputData(input);
  this->SliceAlongZ->SetInputData(input);

  // Update the internal pipeline
  this->CombinedFilteredInput->Update();

  // Copy generated output to filter output
  output->ShallowCopy(this->CombinedFilteredInput->GetOutput());

  return 1;
}
