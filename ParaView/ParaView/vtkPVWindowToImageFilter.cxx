/*=========================================================================

  Module:    vtkPVWindowToImageFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWindowToImageFilter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderModule.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkPVWindowToImageFilter, "1.1");
vtkStandardNewMacro(vtkPVWindowToImageFilter);

vtkPVWindowToImageFilter::vtkPVWindowToImageFilter()
{
  this->Input = NULL;
  this->ReadFrontBuffer = 1;
  this->ShouldRender = 1;
}

vtkPVWindowToImageFilter::~vtkPVWindowToImageFilter()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

void vtkPVWindowToImageFilter::SetInput(vtkPVRenderModule *input)
{
  if (input != this->Input)
    {
    if (this->Input)
      {
      this->Input->UnRegister(this);
      }
    this->Input = input;
    if (this->Input)
      {
      this->Input->Register(this);
      }
    this->Modified();
    }
}

void vtkPVWindowToImageFilter::ExecuteInformation()
{
  if (this->Input == NULL)
    {
    vtkErrorMacro("Please specify a vtkPVRenderModule as input!");
    return;
    }
  
  vtkImageData *out = this->GetOutput();
  
  // set the extent
  out->SetWholeExtent(0,
                      this->Input->GetRenderWindow()->GetSize()[0] - 1,
                      0,
                      this->Input->GetRenderWindow()->GetSize()[1] - 1,
                      0, 0);
  
  // set the spacing
  out->SetSpacing(1.0, 1.0, 1.0);
  
  // set the origin
  out->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  out->SetNumberOfScalarComponents(3);
  out->SetScalarType(VTK_UNSIGNED_CHAR);
}

void vtkPVWindowToImageFilter::ExecuteData(vtkDataObject *)
{
  vtkImageData *out = this->GetOutput();
  out->SetExtent(out->GetWholeExtent());
  out->AllocateScalars();
  
  int outIncrY;
  int size[2];
  unsigned char *pixels, *outPtr;
  int idxY, rowSize;
  
  if (out->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // get the size of the render window
  size[0] = this->Input->GetRenderWindow()->GetSize()[0];
  size[1] = this->Input->GetRenderWindow()->GetSize()[1];
  rowSize = size[0]*3;
  outIncrY = size[0]*3;
  
  vtkRenderWindow *renWin =
    vtkRenderWindow::SafeDownCast(this->Input->GetRenderWindow());
  
  if (this->ShouldRender)
    {
    this->Input->StillRender();
    }
  
  int buffer = this->ReadFrontBuffer;
  if(!renWin->GetDoubleBuffer())
    {
    buffer = 1;
    }      

  pixels = renWin->GetPixelData(0,0,size[0] - 1, size[1] - 1, buffer);
  unsigned char *pixels1 = pixels;
      
  // now write the data to the output image
  outPtr = 
    (unsigned char *)out->GetScalarPointer(0,0, 0);
  
  // Loop through ouput pixels
  for (idxY = 0; idxY < size[1]; idxY++)
    {
    memcpy(outPtr,pixels1,rowSize);
    outPtr += outIncrY;
    pixels1 += rowSize;
    }
  
  // free the memory
  delete [] pixels;  
}

void vtkPVWindowToImageFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  if (this->Input)
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  os << indent << "ReadFrontBuffer: " << this->ReadFrontBuffer << "\n";
  os << indent << "ShouldRender: " << this->ShouldRender << "\n";
}
