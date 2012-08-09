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
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"

#include <math.h>

vtkStandardNewMacro(vtkThreeSliceFilter);

//----------------------------------------------------------------------------
vtkThreeSliceFilter::vtkThreeSliceFilter()
{
  // Setup output ports [ Merge, SliceX, SliceY, SliceZ]
  this->SetNumberOfOutputPorts(4);

  // Setup Merge filter
  this->CombinedFilteredInput = vtkAppendPolyData::New();

  for(int i = 0; i < 3; ++i)
    {
    // Allocate internal vars
    this->Slices[i] = vtkCutter::New();
    this->Planes[i] = vtkPlane::New();
    this->Slices[i]->SetCutFunction(this->Planes[i]);

    // Bind pipeline
    this->CombinedFilteredInput->AddInputConnection(this->Slices[i]->GetOutputPort());
    }

  this->SetToDefaultSettings();
}

//----------------------------------------------------------------------------
vtkThreeSliceFilter::~vtkThreeSliceFilter()
{
  for(int i=0; i < 3; ++i)
    {
    this->Slices[i]->Delete();
    this->Slices[i] = NULL;
    this->Planes[i]->Delete();
    this->Planes[i] = NULL;
    }

  this->CombinedFilteredInput->Delete();
  this->CombinedFilteredInput = NULL;
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetToDefaultSettings()
{
  double origin[3] = {0,0,0};
  for(int i = 0; i < 3; ++i)
    {
    // Setup normal
    double normal[3] = {0,0,0};
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
unsigned long vtkThreeSliceFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long internalMTime = 0;

  // Test Append filter
  internalMTime = this->CombinedFilteredInput->GetMTime();
  mTime = ( internalMTime > mTime ? internalMTime : mTime );

  // Test slices
  for(int i = 0; i < 3; ++i)
    {
    internalMTime = this->Slices[i]->GetMTime();
    mTime = ( internalMTime > mTime ? internalMTime : mTime );
    }

  return mTime;
}
//----------------------------------------------------------------------------
void vtkThreeSliceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for(int i=0; i < 3; ++i)
    {
    os << indent << " - Plane[" << i << "]: normal("
       << this->Planes[i]->GetNormal()[0] << ", "
       << this->Planes[i]->GetNormal()[1] << ", "
       << this->Planes[i]->GetNormal()[2] << ") - origin("
       << this->Planes[i]->GetOrigin()[0] << ", "
       << this->Planes[i]->GetOrigin()[1] << ", "
       << this->Planes[i]->GetOrigin()[2] << ")\n";
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
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetCutValue(int cutIndex, int index, double value)
{
  vtkCutter* slice = this->Slices[cutIndex];
  if(slice->GetValue(index) != value)
    {
    slice->SetValue(index, value);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkThreeSliceFilter::SetNumberOfSlice(int cutIndex, int size)
{
  vtkCutter* slice = this->Slices[cutIndex];
  if(size != slice->GetNumberOfContours())
    {
    slice->SetNumberOfContours(size);
    this->Modified();
    }
}
//-----------------------------------------------------------------------
int vtkThreeSliceFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
//-----------------------------------------------------------------------
int vtkThreeSliceFilter::RequestData( vtkInformation *,
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get input
  vtkDataSet *input =
      vtkDataSet::SafeDownCast(
        inputVector[0]->GetInformationObject(0)->Get(
          vtkDataObject::DATA_OBJECT()));

  // get outputs
  vtkPolyData *output[4];
  for(int i=0; i < 4; ++i)
    {
    output[i] =
        vtkPolyData::SafeDownCast(
          outputVector->GetInformationObject(i)->Get(
            vtkDataObject::DATA_OBJECT()));
    }

  // Add CellIds to allow cell selection to work
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

  // Setup internal pipeline
  this->Slices[0]->SetInputData(input);
  this->Slices[1]->SetInputData(input);
  this->Slices[2]->SetInputData(input);

  // Update the internal pipeline
  this->CombinedFilteredInput->Update();

  // Copy generated output to filter output
  output[0]->ShallowCopy(this->CombinedFilteredInput->GetOutput());
  output[1]->ShallowCopy(this->Slices[0]->GetOutput());
  output[2]->ShallowCopy(this->Slices[1]->GetOutput());
  output[3]->ShallowCopy(this->Slices[2]->GetOutput());

  return 1;
}
