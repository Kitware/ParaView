/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransferFunction2D.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// ParaView includes
#include "vtkPVTransferFunction2D.h"

#include "vtkPVTransferFunction2DBox.h"

// VTK includes
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

// STL includes
#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkPVTransferFunction2D);

//-------------------------------------------------------------------------------------------------
class vtkPVTransferFunction2DInternals
{
public:
  /**
   * Vector of boxes
   */
  std::vector<vtkPVTransferFunction2DBox*> Boxes;

  /**
   * The (xmin, xmax, ymin, ymax) box locations.
   */
  double Range[4] = { 0, 0, 0, 0 };

  /**
   * The transfer function
   */
  vtkSmartPointer<vtkImageData> Function;
};

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2D::vtkPVTransferFunction2D()
{
  this->Internals = new vtkPVTransferFunction2DInternals();
}

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2D::~vtkPVTransferFunction2D()
{
  if (this->Internals->Function)
  {
    this->Internals->Function->Delete();
    this->Internals->Function = nullptr;
  }
  for (auto box : this->Internals->Boxes)
  {
    if (box)
    {
      box->UnRegister(this);
    }
  }
  this->Internals->Boxes.clear();
  delete this->Internals;
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::PrintSelf(ostream& os, vtkIndent indent)
{
  // os << indent << " = " << this-> << endl;
}

//------------------------------------------------------------------------------------------------
bool vtkPVTransferFunction2D::UpdateRange()
{
  if (this->UseCustomRange)
  {
    this->Internals->Range[0] = this->CustomRange[0];
    this->Internals->Range[1] = this->CustomRange[1];
    this->Internals->Range[2] = this->CustomRange[2];
    this->Internals->Range[3] = this->CustomRange[3];
    return false;
  }

  double oldRange[4];
  oldRange[0] = this->Internals->Range[0];
  oldRange[1] = this->Internals->Range[1];
  oldRange[2] = this->Internals->Range[2];
  oldRange[3] = this->Internals->Range[3];
  if (this->Internals->Boxes.empty())
  {
    this->Internals->Range[0] = 0;
    this->Internals->Range[1] = 0;
    this->Internals->Range[2] = 0;
    this->Internals->Range[3] = 0;
  }
  else
  {
    auto BoxXMin = std::min_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetX() < box2->GetBox().GetX();
      });
    this->Internals->Range[0] = (*BoxXMin)->GetBox().GetX();
    auto BoxXMax = std::max_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetX() + box1->GetBox().GetWidth() <
          box2->GetBox().GetX() + box2->GetBox().GetWidth();
      });
    this->Internals->Range[1] = (*BoxXMax)->GetBox().GetX() + (*BoxXMax)->GetBox().GetWidth();
    auto BoxYMin = std::min_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetY() < box2->GetBox().GetY();
      });
    this->Internals->Range[2] = (*BoxYMin)->GetBox().GetY();
    auto BoxYMax = std::max_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetY() + box1->GetBox().GetHeight() <
          box2->GetBox().GetY() + box2->GetBox().GetHeight();
      });
    this->Internals->Range[3] = (*BoxYMax)->GetBox().GetY() + (*BoxYMax)->GetBox().GetHeight();
  }

  if (oldRange[0] == this->Internals->Range[0] && oldRange[1] == this->Internals->Range[1] &&
    oldRange[2] == this->Internals->Range[2] && oldRange[3] == this->Internals->Range[3])
  {
    return false;
  }

  this->Modified();
  return true;
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::Build()
{
  if (this->Internals->Boxes.empty())
  {
    return;
  }

  if (!this->Internals->Function)
  {
    this->Internals->Function = vtkSmartPointer<vtkImageData>::New();
  }

  if (auto oldScalars = this->Internals->Function->GetPointData()->GetScalars())
  {
    this->Internals->Function->GetPointData()->RemoveArray(oldScalars->GetName());
  }

  this->UpdateRange();

  auto func = this->Internals->Function;
  func->SetOrigin(this->Internals->Range[0], this->Internals->Range[2], 0.0);
  func->SetDimensions(this->OutputDimensions[0], this->OutputDimensions[1], 1);
  double spacing[2];
  spacing[0] = (this->Internals->Range[1] - this->Internals->Range[0]) / this->OutputDimensions[0];
  spacing[1] = (this->Internals->Range[3] - this->Internals->Range[3]) / this->OutputDimensions[1];
  func->SetSpacing(spacing[0], spacing[1], 1.0);
  func->AllocateScalars(VTK_FLOAT, 4);
  auto arr = vtkFloatArray::SafeDownCast(func->GetPointData()->GetScalars());
  auto arrRange = vtk::DataArrayValueRange(arr);
  std::fill(arrRange.begin(), arrRange.end(), 0.0);

  for (const auto box : this->Internals->Boxes)
  {
    if (!box)
    {
      continue;
    }
    const vtkRectd bbox = box->GetBox();
    vtkIdType width = static_cast<vtkIdType>(bbox.GetWidth() / spacing[0]);
    vtkIdType height = static_cast<vtkIdType>(bbox.GetHeight() / spacing[1]);
    if (width <= 0 || height <= 0)
    {
      continue;
    }
    vtkIdType x0 = static_cast<vtkIdType>((bbox.GetX() - this->Internals->Range[0]) / spacing[0]);
    vtkIdType y0 = static_cast<vtkIdType>((bbox.GetY() - this->Internals->Range[2]) / spacing[1]);
    x0 = bbox.GetX() <= this->Internals->Range[0] ? 0 : x0;
    x0 = bbox.GetX() >= this->Internals->Range[1] ? this->OutputDimensions[0] : x0;
    y0 = bbox.GetY() <= this->Internals->Range[2] ? 0 : y0;
    y0 = bbox.GetY() >= this->Internals->Range[3] ? this->OutputDimensions[1] : y0;
    vtkIdType x1 =
      static_cast<vtkIdType>((bbox.GetRight() - this->Internals->Range[0]) / spacing[0]);
    vtkIdType y1 = static_cast<vtkIdType>((bbox.GetTop() - this->Internals->Range[2]) / spacing[1]);
    x1 = bbox.GetRight() <= this->Internals->Range[0] ? 0 : x1;
    x1 = bbox.GetRight() >= this->Internals->Range[1] ? this->OutputDimensions[0] : x1;
    y1 = bbox.GetTop() <= this->Internals->Range[2] ? 0 : y1;
    x1 = bbox.GetTop() >= this->Internals->Range[3] ? this->OutputDimensions[1] : y1;
    vtkSmartPointer<vtkImageData> boxTexture = box->GetTexture();
    int boxDims[3];
    boxTexture->GetDimensions(boxDims);
    double boxSpacing[3] = { 1, 1, 1 };
    boxSpacing[0] = bbox.GetWidth() / boxDims[0];
    boxSpacing[1] = bbox.GetHeight() / boxDims[1];
    for (vtkIdType ii = x0; ii < x1; ++ii)
    {
      for (vtkIdType jj = y0; jj < y1; ++jj)
      {
        int boxCoord[3] = { 0, 0, 0 };
        boxCoord[0] =
          (ii * spacing[0] + this->Internals->Range[0] - bbox.GetLeft()) / boxSpacing[0];
        boxCoord[1] =
          (jj * spacing[1] + this->Internals->Range[2] - bbox.GetBottom()) / boxSpacing[1];
        unsigned char* ptr = static_cast<unsigned char*>(boxTexture->GetScalarPointer(boxCoord));
        float fptr[4];
        for (int tp = 0; tp < 4; ++tp)
        {
          fptr[tp] = ptr[tp] / 255.0;
        }
        // composite this color with the current color
        float* c = static_cast<float*>(func->GetScalarPointer(ii, jj, 0));
        for (int tp = 0; tp < 3; ++tp)
        {
          c[tp] = fptr[tp] + c[tp] * (1 - fptr[3]);
        }
        c[3] = fptr[3] + c[3] * (1 - fptr[3]);
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::AddControlBox(vtkPVTransferFunction2DBox* box)
{
  if (!box)
  {
    return -1;
  }
  this->Internals->Boxes.push_back(box);
  // If range is updated, modified has been invoked -> don't invoke again.
  bool modifiedInvoked = this->UpdateRange();
  if (!modifiedInvoked)
  {
    this->Modified();
  }
  return static_cast<int>(this->Internals->Boxes.size()) - 1;
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::AddControlBox(
  double x, double y, double width, double height, double r, double g, double b, double a)
{
  vtkNew<vtkPVTransferFunction2DBox> tfBox;
  tfBox->SetBox(x, y, width, height);
  tfBox->SetColor(r, g, b, a);
  return this->AddControlBox(tfBox);
}
//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::AddControlBox(
  double x, double y, double width, double height, double* color)
{
  vtkRectd box(x, y, width, height);
  return this->AddControlBox(box, color);
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::AddControlBox(vtkRectd& box, double* color)
{
  if (!color)
  {
    return -1;
  }

  vtkNew<vtkPVTransferFunction2DBox> tfBox;
  tfBox->SetBox(box);
  tfBox->SetColor(color);
  return this->AddControlBox(tfBox);
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::RemoveControlBox(int id)
{
  if (static_cast<int>(this->Internals->Boxes.size()) <= id)
  {
    return -1;
  }
  this->Internals->Boxes.erase(this->Internals->Boxes.begin() + id);
  // If range is updated, modified has been invoked -> don't invoke again.
  bool modifiedInvoked = this->UpdateRange();
  if (!modifiedInvoked)
  {
    this->Modified();
  }
  return id;
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::RemoveControlBox(vtkPVTransferFunction2DBox* box)
{
  int n = -1;
  auto it = std::find(this->Internals->Boxes.cbegin(), this->Internals->Boxes.cend(), box);
  if (it != this->Internals->Boxes.cend())
  {
    n = std::distance(this->Internals->Boxes.cbegin(), it);
    return this->RemoveControlBox(n);
  }
  return n;
}

//-------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::RemoveAllBoxes()
{
  this->Internals->Boxes.clear();
  // If range is updated, modified has been invoked -> don't invoke again.
  bool modifiedInvoked = this->UpdateRange();
  if (!modifiedInvoked)
  {
    this->Modified();
  }
}

//-------------------------------------------------------------------------------------------------
double* vtkPVTransferFunction2D::GetRange()
{
  return this->Internals->Range;
}

//-------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::GetRange(double& arg1, double& arg2, double& arg3, double& arg4)
{
  arg1 = this->Internals->Range[0];
  arg2 = this->Internals->Range[1];
  arg3 = this->Internals->Range[2];
  arg4 = this->Internals->Range[3];
}

//-------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::GetRange(double arg[4])
{
  this->GetRange(arg[0], arg[1], arg[2], arg[3]);
}
