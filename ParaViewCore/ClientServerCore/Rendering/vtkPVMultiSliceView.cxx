/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiSliceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiSliceView.h"

#include "vtkCommand.h"
#include "vtkContourValues.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

// ==========================================================================
#define SliceMethodMacro(axes,index)                                           \
int vtkPVMultiSliceView::GetNumberOfSlice##axes() const                  \
  { return this->GetNumberOfSlice(index); }                                    \
void vtkPVMultiSliceView::SetNumberOfSlice##axes(int size)                     \
  { this->SetNumberOfSlice(index,size); }                                      \
void vtkPVMultiSliceView::SetSlice##axes(int i, double value)                  \
  { this->SetSlice(index, i, value); }                                         \
const double* vtkPVMultiSliceView::GetSlice##axes() const                      \
  { return this->GetSlice(index); }                                            \
void vtkPVMultiSliceView::SetSlice##axes##Origin(double x, double y, double z) \
  { this->SetSliceOrigin(index,x,y,z); }                                       \
const double* vtkPVMultiSliceView::GetSlice##axes##Origin() const              \
  { return this->GetSliceOrigin(index); }                                      \
void vtkPVMultiSliceView::SetSlice##axes##Normal(double x, double y, double z) \
{ this->SetSliceNormal(index,x,y,z); }                                         \
const double* vtkPVMultiSliceView::GetSlice##axes##Normal() const              \
{ return this->GetSliceNormal(index); }

// ==========================================================================
SliceMethodMacro(X,0);
SliceMethodMacro(Y,1);
SliceMethodMacro(Z,2);
// ==========================================================================
namespace {
  struct SliceData
  {
    vtkNew<vtkContourValues> Values;
    double Origin[3];
    double Normal[3];

    SliceData()
    {
    this->Values->SetNumberOfContours(0);
    }
  };
}

//----------------------------------------------------------------------------
class vtkPVMultiSliceView::vtkSliceInternal
{
public:
  SliceData Slices[3];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiSliceView);
//----------------------------------------------------------------------------
vtkPVMultiSliceView::vtkPVMultiSliceView()
{
  this->ShowOutline = 1;
  this->Internal = new vtkSliceInternal();
}

//----------------------------------------------------------------------------
vtkPVMultiSliceView::~vtkPVMultiSliceView()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVMultiSliceView::GetNumberOfSlice(int sliceIndex) const
{
  return this->Internal->Slices[sliceIndex].Values->GetNumberOfContours();
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetNumberOfSlice(int sliceIndex, int size)
{
  vtkContourValues* values = this->Internal->Slices[sliceIndex].Values.GetPointer();
  unsigned long before = values->GetMTime();
  values->SetNumberOfContours(size);
  unsigned long after = values->GetMTime();

  if(before != after)
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent);
    }
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetSlice(int sliceIndex, int index, double value)
{
  vtkContourValues* values = this->Internal->Slices[sliceIndex].Values.GetPointer();
  unsigned long before = values->GetMTime();
  values->SetValue(index, value);
  unsigned long after = values->GetMTime();

  if(before != after)
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent);
    }
}

//----------------------------------------------------------------------------
const double* vtkPVMultiSliceView::GetSlice(int sliceIndex) const
{
  return this->Internal->Slices[sliceIndex].Values->GetValues();
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetSliceOrigin(int sliceIndex, double x, double y, double z)
{
  double* origin = this->Internal->Slices[sliceIndex].Origin;
  bool changed = (origin[0] != x) || (origin[1] != y) || (origin[2] != z);
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;

  if(changed)
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent);
    }
}

//----------------------------------------------------------------------------
double* vtkPVMultiSliceView::GetSliceOrigin(int sliceIndex) const
{
  return this->Internal->Slices[sliceIndex].Origin;
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetSliceNormal(int sliceIndex, double x, double y, double z)
{
  double* normal = this->Internal->Slices[sliceIndex].Normal;
  bool changed = (normal[0] != x) || (normal[1] != y) || (normal[2] != z);
  normal[0] = x;
  normal[1] = y;
  normal[2] = z;

  if(changed)
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent);
    }
}

//----------------------------------------------------------------------------
double* vtkPVMultiSliceView::GetSliceNormal(int sliceIndex) const
{
  return this->Internal->Slices[sliceIndex].Normal;
}
//----------------------------------------------------------------------------
