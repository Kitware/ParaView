/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGridAxes3DActor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGridAxes3DActor.h"

#include "vtkBoundingBox.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVGridAxes3DActor);
//----------------------------------------------------------------------------
vtkPVGridAxes3DActor::vtkPVGridAxes3DActor()
{
  this->DataScale[0] = this->DataScale[1] = this->DataScale[2] = 1.0;
  this->DataPosition[0] = this->DataPosition[1] = this->DataPosition[2] = 0.0;
  this->DataBoundsScaleFactor = 1;

  this->TransformedBounds[0] = this->TransformedBounds[2] = this->TransformedBounds[4] = -1.0;
  this->TransformedBounds[1] = this->TransformedBounds[3] = this->TransformedBounds[5] = 1.0;

  this->UseModelTransform = false;
  this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = -1.0;
  this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = 1.0;
  this->ModelTransformMatrix->Identity();

  this->UseCustomTransformedBounds = false;
  this->CustomTransformedBounds[0] = this->CustomTransformedBounds[2] =
    this->CustomTransformedBounds[4] = -1.0;
  this->CustomTransformedBounds[1] = this->CustomTransformedBounds[3] =
    this->CustomTransformedBounds[5] = 1.0;
}

//----------------------------------------------------------------------------
vtkPVGridAxes3DActor::~vtkPVGridAxes3DActor() = default;

//----------------------------------------------------------------------------
double* vtkPVGridAxes3DActor::GetBounds()
{
  this->UpdateGridBounds();
  return this->Superclass::GetBounds();
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::SetModelTransformMatrix(double* matrix)
{
  if (!std::equal(matrix, matrix + 16, &this->ModelTransformMatrix->Element[0][0]))
  {
    std::copy(matrix, matrix + 16, &this->ModelTransformMatrix->Element[0][0]);
    this->ModelTransformMatrix->Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::Update(vtkViewport* viewport)
{
  this->UpdateGridBounds();
  this->Superclass::Update(viewport);
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBounds()
{
  if (this->BoundsUpdateTime < this->GetMTime())
  {
    if (this->UseModelTransform)
    {
      this->UpdateGridBoundsUsingModelTransform();
    }
    else
    {
      this->UpdateGridBoundsUsingDataBounds();
    }
    this->BoundsUpdateTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBoundsUsingModelTransform()
{
  this->SetPosition(0, 0, 0);
  this->SetScale(1, 1, 1);
  this->SetGridBounds(this->ModelBounds);
  this->SetUserMatrix(this->ModelTransformMatrix.Get());
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBoundsUsingDataBounds()
{
  vtkBoundingBox bbox(
    this->UseCustomTransformedBounds ? this->CustomTransformedBounds : this->TransformedBounds);
  if (bbox.IsValid())
  {
    if (this->DataPosition[0] != 0 || this->DataPosition[1] != 0 || this->DataPosition[2] != 0)
    {
      double bds[6];
      bbox.GetBounds(bds);
      bds[0] -= this->DataPosition[0];
      bds[1] -= this->DataPosition[0];
      bds[2] -= this->DataPosition[1];
      bds[3] -= this->DataPosition[1];
      bds[4] -= this->DataPosition[2];
      bds[5] -= this->DataPosition[2];
      bbox.SetBounds(bds);
      this->SetPosition(this->DataPosition);
    }
    else
    {
      this->SetPosition(0, 0, 0);
    }
    if (this->DataScale[0] != 1 || this->DataScale[1] != 1 || this->DataScale[2] != 1)
    {
      bbox.Scale(1.0 / this->DataScale[0], 1.0 / this->DataScale[1], 1.0 / this->DataScale[2]);
      this->SetScale(this->DataScale);
    }
    else
    {
      this->SetScale(1, 1, 1);
    }

    double center[3];
    double bds[6];
    bbox.GetBounds(bds);

    // correct the bounds for the data bounds scale factor
    for (int i = 0; i < 3; i++)
    {
      center[i] = (bds[2 * i] + bds[2 * i + 1]) / 2;
      bds[2 * i] = center[i] - ((center[i] - bds[2 * i]) * this->DataBoundsScaleFactor);
      bds[2 * i + 1] = center[i] + ((bds[2 * i + 1] - center[i]) * this->DataBoundsScaleFactor);
    }

    this->SetGridBounds(bds);
  }
  else
  {
    this->SetGridBounds(
      this->UseCustomTransformedBounds ? this->CustomTransformedBounds : this->TransformedBounds);
    this->SetPosition(0, 0, 0);
    this->SetScale(1, 1, 1);
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::ShallowCopy(vtkProp* prop)
{
  this->Superclass::ShallowCopy(prop);
  if (vtkPVGridAxes3DActor* other = vtkPVGridAxes3DActor::SafeDownCast(prop))
  {
    this->SetDataScale(other->GetDataScale());
    this->SetDataPosition(other->GetDataPosition());
    this->SetTransformedBounds(other->GetTransformedBounds());
    this->SetUseModelTransform(other->GetUseModelTransform());
    this->SetModelBounds(other->GetModelBounds());
    this->SetModelTransformMatrix(reinterpret_cast<double*>(other->ModelTransformMatrix->Element));
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
