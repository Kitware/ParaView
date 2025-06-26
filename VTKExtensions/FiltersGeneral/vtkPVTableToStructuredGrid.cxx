// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVTableToStructuredGrid.h"

vtkStandardNewMacro(vtkPVTableToStructuredGrid);

vtkPVTableToStructuredGrid::vtkPVTableToStructuredGrid()
{
  for (int i = 0; i < 3; i++)
  {
    this->Dimensions[i] = 0;
    this->MinimumIndex[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVTableToStructuredGrid::SetDimensions(int xdim, int ydim, int zdim)
{
  this->Dimensions[0] = xdim;
  this->Dimensions[1] = ydim;
  this->Dimensions[2] = zdim;

  this->WholeExtent[1] = this->WholeExtent[0] + xdim - 1;
  this->WholeExtent[3] = this->WholeExtent[2] + ydim - 1;
  this->WholeExtent[5] = this->WholeExtent[4] + zdim - 1;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVTableToStructuredGrid::SetMinimumIndex(int xmin, int ymin, int zmin)
{
  this->MinimumIndex[0] = xmin;
  this->MinimumIndex[1] = ymin;
  this->MinimumIndex[2] = zmin;

  this->WholeExtent[0] = xmin;
  this->WholeExtent[1] = xmin + this->Dimensions[0] - 1;
  this->WholeExtent[2] = ymin;
  this->WholeExtent[3] = ymin + this->Dimensions[1] - 1;
  this->WholeExtent[4] = zmin;
  this->WholeExtent[5] = zmin + this->Dimensions[2] - 1;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVTableToStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions: (" << this->Dimensions[0];
  for (int idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->Dimensions[idx];
  }
  os << ")\n";
  os << indent << "MinimumIndex: (" << this->MinimumIndex[0];
  for (int idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->MinimumIndex[idx];
  }
  os << ")\n";
}
