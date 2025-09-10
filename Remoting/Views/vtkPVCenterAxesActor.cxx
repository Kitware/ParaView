// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCenterAxesActor.h"

#include "vtkAxes.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"

vtkStandardNewMacro(vtkPVCenterAxesActor);
//----------------------------------------------------------------------------
vtkPVCenterAxesActor::vtkPVCenterAxesActor()
{
  this->Axes = vtkAxes::New();
  this->Axes->SetSymmetric(1);
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Axes->GetOutputPort());

  this->LUT->SetNumberOfTableValues(3);
  this->LUT->SetRange(0.0, 0.5);              // match the values set in vtkAxes::RequestData()
  this->LUT->SetTableValue(0, 1.0, 0.0, 0.0); // color x-axis in red
  this->LUT->SetTableValue(1, 1.0, 1.0, 0.0); // color y-axis in yellow
  this->LUT->SetTableValue(2, 0.0, 0.0, 1.0); // color z-axis in green

  this->Mapper->SetLookupTable(this->LUT);
  this->Mapper->SelectColorArray("Axes");
  this->Mapper->SetUseLookupTableScalarRange(true);

  this->SetMapper(this->Mapper);
  // We disable this, since it results in the center axes being skipped when
  // IceT is rendering.
  // this->SetUseBounds(0); // don't use bounds of this actor in renderer bounds
  // computations.
}

//----------------------------------------------------------------------------
vtkPVCenterAxesActor::~vtkPVCenterAxesActor()
{
  this->Axes->Delete();
  this->Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetSymmetric(int val)
{
  this->Axes->SetSymmetric(val);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetComputeNormals(int val)
{
  this->Axes->SetComputeNormals(val);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetXAxisColor(double r, double g, double b)
{
  this->SetAxisColor(0, r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetYAxisColor(double r, double g, double b)
{
  this->SetAxisColor(1, r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetZAxisColor(double r, double g, double b)
{
  this->SetAxisColor(2, r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetAxisColor(int axis, double r, double g, double b)
{
  double currentColor[4];
  this->LUT->GetTableValue(axis, currentColor);
  if (currentColor[0] != r || currentColor[1] != g || currentColor[2] != b)
  {
    this->LUT->SetTableValue(axis, r, g, b);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
