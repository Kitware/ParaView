/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransferFunction2DBox.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// ParaView includes
#include "vtkPVTransferFunction2DBox.h"

// VTK includes
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkPVTransferFunction2DBox);

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2DBox::vtkPVTransferFunction2DBox()
{
  this->Box.SetX(0);
  this->Box.SetY(0);
  this->Box.SetWidth(1);
  this->Box.SetHeight(1);
}

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2DBox::~vtkPVTransferFunction2DBox()
{
  if (this->Texture)
  {
    this->Texture->Delete();
    this->Texture = nullptr;
  }
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2DBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Box [xmin, xmax, ymin, ymax]: [" << this->Box.GetLeft() << ", "
     << this->Box.GetRight() << ", " << this->Box.GetBottom() << ", " << this->Box.GetTop() << "]"
     << endl;
  os << indent << "Color [r, g, b, a] [" << this->Color[0] << ", " << this->Color[1] << ", "
     << this->Color[2] << ", " << this->Color[3] << "]" << endl;
  os << indent << "Texture Size [width, height]: [" << this->TextureSize[0] << ", "
     << this->TextureSize[1] << "]" << endl;
  os << indent << "GaussianSigmaFactor: " << this->GaussianSigmaFactor << endl;
  os << indent << "Texture: ";
  if (this->Texture)
  {
    this->Texture->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(nullptr)" << endl;
  }
}

//------------------------------------------------------------------------------------------------
const vtkRectd& vtkPVTransferFunction2DBox::GetBox()
{
  return this->Box;
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2DBox::SetBox(
  const double x, const double y, const double width, const double height)
{
  if (x == this->Box.GetLeft() && (x + width) == this->Box.GetRight() &&
    y == this->Box.GetBottom() && (y + height) == this->Box.GetTop())
  {
    return;
  }
  this->Box.SetX(x);
  this->Box.SetY(y);
  this->Box.SetWidth(width);
  this->Box.SetHeight(height);
  this->Modified();
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2DBox::SetBox(const vtkRectd& b)
{
  this->SetBox(b.GetX(), b.GetY(), b.GetWidth(), b.GetHeight());
}

//------------------------------------------------------------------------------------------------
vtkImageData* vtkPVTransferFunction2DBox::GetTexture()
{
  this->ComputeTexture();
  return this->Texture;
}

//------------------------------------------------------------------------------------------------
void vtkPVTransferFunction2DBox::ComputeTexture()
{
  if (this->Texture && this->Texture->GetMTime() > this->GetMTime())
  {
    return;
  }

  if (!this->Texture)
  {
    this->Texture = vtkImageData::New();
  }

  this->Texture->GetPointData()->RemoveArray("Transfer2DBoxScalars");

  this->Texture->SetDimensions(this->TextureSize[0], this->TextureSize[1], 1);
  this->Texture->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  auto arr = vtkUnsignedCharArray::FastDownCast(this->Texture->GetPointData()->GetScalars());
  arr->SetName("Transfer2DBoxScalars");
  const auto dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, this->TextureSize[0] * this->TextureSize[1] * 4 * sizeof(unsigned char));

  double colorC[3] = { 1.0, 1.0, 1.0 };
  colorC[0] = this->Color[0] * 255;
  colorC[1] = this->Color[1] * 255;
  colorC[2] = this->Color[2] * 255;
  double amp = this->Color[3] * 255;
  double sigma =
    this->GaussianSigmaFactor / 100 * (this->TextureSize[0] + this->TextureSize[1]) / 2;
  double expdenom = 2 * sigma * sigma;

  for (int i = 0; i < this->TextureSize[0]; ++i)
  {
    for (int j = 0; j < this->TextureSize[1]; ++j)
    {
      double color[4] = { colorC[0], colorC[1], colorC[2], 0.0 };
      double e = -1.0 *
        ((i - this->TextureSize[0] / 2.0) * (i - this->TextureSize[0] / 2.0) +
          (j - this->TextureSize[1] / 2.0) * (j - this->TextureSize[1] / 2.0)) /
        expdenom;
      color[3] = amp * std::exp(e);
      arr->SetTuple(i * this->TextureSize[1] + j, color);
    }
  }
  this->Texture->Modified();
}
