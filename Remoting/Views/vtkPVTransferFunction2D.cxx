// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// ParaView includes
#include "vtkPVTransferFunction2D.h"

#include "vtkPVTransferFunction2DBox.h"

// VTK includes
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

// STL includes
#include <algorithm>

vtkStandardNewMacro(vtkPVTransferFunction2D);

//-------------------------------------------------------------------------------------------------
class vtkPVTransferFunction2DInternals
{
public:
  /**
   * Vector of boxes
   */
  std::vector<vtkSmartPointer<vtkPVTransferFunction2DBox>> Boxes;

  /**
   * The transfer function
   */
  vtkImageData* Function = nullptr;
  vtkTimeStamp BuildTime;
};

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2D::vtkPVTransferFunction2D()
  : Internals(new vtkPVTransferFunction2DInternals())
{
}

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2D::~vtkPVTransferFunction2D()
{
  if (this->Internals->Function != nullptr)
  {
    this->Internals->Function->UnRegister(this);
    this->Internals->Function = nullptr;
  }
  for (auto box : this->Internals->Boxes)
  {
    if (box)
    {
      box = nullptr;
    }
  }
  this->Internals->Boxes.clear();
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Range = (" << this->Range[0] << ", " << this->Range[1] << ", " << this->Range[2]
     << ", " << this->Range[3] << ")" << endl;
  os << indent << "OutputDimensions = (" << this->OutputDimensions[0] << ", "
     << this->OutputDimensions[1] << ")" << endl;
  os << indent << "Function: ";
  if (this->Internals->Function)
  {
    this->Internals->Function->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(nullptr)" << endl;
  }
  os << "Boxes: " << (!this->Internals->Boxes.empty() ? "" : "(None)");
  for (auto box : this->Internals->Boxes)
  {
    box->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------------------------
bool vtkPVTransferFunction2D::UpdateRange()
{
  double oldRange[4];
  oldRange[0] = this->Range[0];
  oldRange[1] = this->Range[1];
  oldRange[2] = this->Range[2];
  oldRange[3] = this->Range[3];
  if (this->Internals->Boxes.empty())
  {
    this->Range[0] = 0;
    this->Range[1] = 0;
    this->Range[2] = 0;
    this->Range[3] = 0;
  }
  else
  {
    auto BoxXMin = std::min_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetX() < box2->GetBox().GetX();
      });
    this->Range[0] = (*BoxXMin)->GetBox().GetX();
    auto BoxXMax = std::max_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetX() + box1->GetBox().GetWidth() <
          box2->GetBox().GetX() + box2->GetBox().GetWidth();
      });
    this->Range[1] = (*BoxXMax)->GetBox().GetX() + (*BoxXMax)->GetBox().GetWidth();
    auto BoxYMin = std::min_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetY() < box2->GetBox().GetY();
      });
    this->Range[2] = (*BoxYMin)->GetBox().GetY();
    auto BoxYMax = std::max_element(this->Internals->Boxes.begin(), this->Internals->Boxes.end(),
      [](vtkPVTransferFunction2DBox* box1, vtkPVTransferFunction2DBox* box2) {
        return box1->GetBox().GetY() + box1->GetBox().GetHeight() <
          box2->GetBox().GetY() + box2->GetBox().GetHeight();
      });
    this->Range[3] = (*BoxYMax)->GetBox().GetY() + (*BoxYMax)->GetBox().GetHeight();
  }

  if (oldRange[0] == this->Range[0] && oldRange[1] == this->Range[1] &&
    oldRange[2] == this->Range[2] && oldRange[3] == this->Range[3])
  {
    return false;
  }

  this->Modified();
  return true;
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::Build()
{
  if (this->Internals->BuildTime > this->GetMTime())
  {
    return;
  }

  if (!this->Internals->Function)
  {
    this->Internals->Function = vtkImageData::New();
  }
  this->Internals->Function->Initialize();

  this->Internals->Function->SetDimensions(this->OutputDimensions[0], this->OutputDimensions[1], 1);
  this->Internals->Function->SetOrigin(this->Range[0], this->Range[2], 0.0);
  double spacing[2];
  spacing[0] = (this->Range[1] - this->Range[0]) / this->OutputDimensions[0];
  spacing[1] = (this->Range[3] - this->Range[2]) / this->OutputDimensions[1];
  this->Internals->Function->SetSpacing(spacing[0], spacing[1], 1.0);
  this->Internals->Function->AllocateScalars(VTK_FLOAT, 4);
  auto arr = vtkFloatArray::FastDownCast(this->Internals->Function->GetPointData()->GetScalars());
  auto arrRange = vtk::DataArrayValueRange(arr);
  std::fill(arrRange.begin(), arrRange.end(), 0.0);

  for (const auto& box : this->Internals->Boxes)
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
    vtkIdType x0 = static_cast<vtkIdType>((bbox.GetX() - this->Range[0]) / spacing[0]);
    vtkIdType y0 = static_cast<vtkIdType>((bbox.GetY() - this->Range[2]) / spacing[1]);
    x0 = bbox.GetX() <= this->Range[0] ? 0 : x0;
    x0 = bbox.GetX() >= this->Range[1] ? this->OutputDimensions[0] : x0;
    y0 = bbox.GetY() <= this->Range[2] ? 0 : y0;
    y0 = bbox.GetY() >= this->Range[3] ? this->OutputDimensions[1] : y0;
    vtkIdType x1 = static_cast<vtkIdType>((bbox.GetRight() - this->Range[0]) / spacing[0]);
    vtkIdType y1 = static_cast<vtkIdType>((bbox.GetTop() - this->Range[2]) / spacing[1]);
    x1 = bbox.GetRight() <= this->Range[0] ? 0 : x1;
    x1 = bbox.GetRight() >= this->Range[1] ? this->OutputDimensions[0] : x1;
    y1 = bbox.GetTop() <= this->Range[2] ? 0 : y1;
    y1 = bbox.GetTop() >= this->Range[3] ? this->OutputDimensions[1] : y1;
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
        boxCoord[0] = (ii * spacing[0] + this->Range[0] - bbox.GetLeft()) / boxSpacing[0];
        boxCoord[0] = boxCoord[0] < 0 ? 0 : boxCoord[0];
        boxCoord[1] = (jj * spacing[1] + this->Range[2] - bbox.GetBottom()) / boxSpacing[1];
        boxCoord[1] = boxCoord[1] < 0 ? 0 : boxCoord[1];
        unsigned char* ptr = static_cast<unsigned char*>(boxTexture->GetScalarPointer(boxCoord));
        float fptr[4];
        for (int tp = 0; tp < 4; ++tp)
        {
          fptr[tp] = ptr[tp] / 255.0;
        }
        // composite this color with the current color
        float* c = static_cast<float*>(this->Internals->Function->GetScalarPointer(ii, jj, 0));
        for (int tp = 0; tp < 3; ++tp)
        {
          c[tp] = fptr[tp] + c[tp] * (1 - fptr[3]);
        }
        c[3] = fptr[3] + c[3] * (1 - fptr[3]);
      }
    }
  }
  this->Internals->BuildTime.Modified();
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::AddControlBox(vtkSmartPointer<vtkPVTransferFunction2DBox> box)
{
  if (!box)
  {
    return -1;
  }
  this->Internals->Boxes.push_back(box);
  this->Modified();
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
int vtkPVTransferFunction2D::SetControlBox(int id, vtkSmartPointer<vtkPVTransferFunction2DBox> b)
{
  if (id < 0)
  {
    // invalid id
    return -1;
  }
  if (id >= static_cast<int>(this->Internals->Boxes.size()))
  {
    return this->AddControlBox(b);
  }

  auto box = this->Internals->Boxes.at(id);
  box->SetBox(b->GetBox());
  box->SetColor(b->GetColor());
  return id;
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::RemoveControlBox(int id)
{
  if (static_cast<int>(this->Internals->Boxes.size()) <= id)
  {
    return -1;
  }
  this->Internals->Boxes.erase(this->Internals->Boxes.begin() + id);
  this->Modified();
  return id;
}

//-------------------------------------------------------------------------------------------------
int vtkPVTransferFunction2D::RemoveControlBox(vtkSmartPointer<vtkPVTransferFunction2DBox> box)
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
std::vector<vtkSmartPointer<vtkPVTransferFunction2DBox>> vtkPVTransferFunction2D::GetBoxes()
{
  return this->Internals->Boxes;
}

//-------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2D::RemoveAllBoxes()
{
  this->Internals->Boxes.clear();
  this->Modified();
}

//------------------------------------------------------------------------------------------------
vtkImageData* vtkPVTransferFunction2D::GetFunction()
{
  if (!this->Internals->Function)
  {
    this->Build();
  }
  return this->Internals->Function;
}
