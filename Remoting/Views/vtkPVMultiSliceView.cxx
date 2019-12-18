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

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVChangeOfBasisHelper.h"
#include "vtkTransform.h"

#include <algorithm>
#include <cassert>

//----------------------------------------------------------------------------
class vtkPVMultiSliceView::vtkSliceInternal
{
public:
  std::vector<double> SliceOffsets[3];
  vtkBoundingBox DataBounds;
  vtkClientServerStream Stream;

  std::pair<bool, std::string> AxisLabels[3];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiSliceView);
//----------------------------------------------------------------------------
vtkPVMultiSliceView::vtkPVMultiSliceView()
  : ModelTransformationMatrix()
{
  this->Internal = new vtkSliceInternal();
  this->ModelTransformationMatrix->Identity();
}

//----------------------------------------------------------------------------
vtkPVMultiSliceView::~vtkPVMultiSliceView()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetModelTransformationMatrix(vtkMatrix4x4* matrix)
{
  if (matrix)
  {
    // Update this->ModelTransformationMatrix is matrix is indeed different.
    if (!std::equal(static_cast<const double*>(&matrix->Element[0][0]),
          static_cast<const double*>(&matrix->Element[0][0]) + 16,
          static_cast<const double*>(&this->ModelTransformationMatrix->Element[0][0])))
    {
      this->ModelTransformationMatrix->DeepCopy(matrix);
    }
  }
  else
  {
    if (this->ModelTransformationMatrix->Determinant() != 1)
    {
      this->ModelTransformationMatrix->Identity();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetNumberOfSlices(int type, unsigned int count)
{
  assert(type >= 0 && type <= 2);
  if (static_cast<unsigned int>(this->Internal->SliceOffsets[type].size()) != count)
  {
    this->Internal->SliceOffsets[type].resize(count);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetSlices(int type, const double* values)
{
  assert(type >= 0 && type <= 2);
  if (std::equal(this->Internal->SliceOffsets[type].begin(),
        this->Internal->SliceOffsets[type].end(), values) == false)
  {
    std::copy(values, values + this->Internal->SliceOffsets[type].size(),
      this->Internal->SliceOffsets[type].begin());
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const std::vector<double>& vtkPVMultiSliceView::GetSlices(int axis) const
{
  assert(axis >= 0 && axis <= 2);
  return this->Internal->SliceOffsets[axis];
}

//----------------------------------------------------------------------------
const char* vtkPVMultiSliceView::GetAxisLabel(int axis) const
{
  assert(axis >= 0 && axis <= 2);
  return this->Internal->AxisLabels[axis].first ? this->Internal->AxisLabels[axis].second.c_str()
                                                : NULL;
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::Update()
{
  this->Internal->DataBounds.Reset();
  this->Internal->AxisLabels[0].first = false;
  this->Internal->AxisLabels[1].first = false;
  this->Internal->AxisLabels[2].first = false;

  this->Superclass::Update();

  vtkBoundingBox result;
  this->AllReduce(this->Internal->DataBounds, result);
  if (result.IsValid())
  {
    this->Internal->DataBounds = result;
  }
  else
  {
    this->Internal->DataBounds.Reset();
  }
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::AboutToRenderOnLocalProcess(bool interactive)
{
  if (this->CenterAxes->GetVisibility() &&
    (this->ModelTransformationMatrix->GetMTime() > this->ModelTransformationMatrixUpdateTime ||
        this->CenterAxes->GetMTime() > this->ModelTransformationMatrixUpdateTime))
  {
    // The CenterAxes is still going to show the position of the center of
    // rotation in Cartesian space. We however want to rotate the u,v,w vectors
    // to point the directions given by the model space.
    vtkVector3d u, v, w;
    vtkPVChangeOfBasisHelper::GetBasisVectors(
      this->ModelTransformationMatrix.GetPointer(), u, v, w);
    u.Normalize();
    v.Normalize();
    w.Normalize();
    vtkSmartPointer<vtkMatrix4x4> rotationMatrix =
      vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(u, v, w);

    // To rotate the axes without changing its position in cartesian space, we
    // apply a linear transform that moves the point to origin, rotates it and
    // then moves it back to the original position.
    double pos[3];
    this->CenterAxes->GetPosition(pos);
    vtkNew<vtkTransform> transform;
    transform->PostMultiply();
    transform->Identity();
    transform->Translate(-pos[0], -pos[1], -pos[2]);
    transform->Concatenate(rotationMatrix.GetPointer());
    transform->Translate(pos);

    this->CenterAxes->SetUserTransform(transform.GetPointer());
    this->ModelTransformationMatrixUpdateTime.Modified();
  }
  this->Superclass::AboutToRenderOnLocalProcess(interactive);
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetAxisTitle(vtkInformation* info, int axis, const char* title)
{
  vtkPVMultiSliceView* view = vtkPVMultiSliceView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  assert(axis >= 0 && axis <= 2);
  view->Internal->AxisLabels[axis].first = true;
  view->Internal->AxisLabels[axis].second = title ? title : "";
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::SetDataBounds(vtkInformation* info, const double bounds[6])
{
  vtkPVMultiSliceView* view = vtkPVMultiSliceView::SafeDownCast(info->Get(VIEW()));
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return;
  }
  view->Internal->DataBounds.AddBounds(const_cast<double*>(bounds));
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::GetDataBounds(double bounds[6]) const
{
  if (this->Internal->DataBounds.IsValid())
  {
    this->Internal->DataBounds.GetBounds(bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(bounds);
  }
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVMultiSliceView::GetAxisLabels() const
{
  vtkClientServerStream& stream = this->Internal->Stream;
  stream.Reset();
  stream << vtkClientServerStream::Reply;
  for (int axis = 0; axis < 3; axis++)
  {
    stream << this->Internal->AxisLabels[axis].first << this->Internal->AxisLabels[axis].second;
  }
  stream << vtkClientServerStream::End;
  return stream;
}

//----------------------------------------------------------------------------
void vtkPVMultiSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
