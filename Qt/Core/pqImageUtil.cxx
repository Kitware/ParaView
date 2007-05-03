/*=========================================================================

   Program:   ParaView
   Module:    pqImageUtil.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqImageUtil.h"
#include <QImage>
#include "vtkImageData.h"

// NOTES:
// - QImage pixel data is 32 bit aligned, whereas vtkImageData's pixel data 
//   is not necessarily 32 bit aligned
// - QImage is a mirror of vtkImageData, which is taken care of by scanning
//   a row at a time and copying opposing rows

bool pqImageUtil::toImageData(const QImage& img, vtkImageData* vtkimage)
{
  int height = img.height();
  int width = img.width();
  int numcomponents = img.hasAlphaChannel() ? 4 : 3;
  
  vtkimage->SetWholeExtent(0, width-1, 0, height-1, 0, 0); 
  vtkimage->SetSpacing(1.0, 1.0, 1.0);
  vtkimage->SetOrigin(0.0, 0.0, 0.0);
  vtkimage->SetNumberOfScalarComponents(numcomponents);
  vtkimage->SetScalarType(VTK_UNSIGNED_CHAR);
  vtkimage->SetExtent(vtkimage->GetWholeExtent());
  vtkimage->AllocateScalars();
  for(int i=0; i<height; i++)
    {
    unsigned char* row;
    row = static_cast<unsigned char*>(vtkimage->GetScalarPointer(0, height-i-1, 0));
    const QRgb* linePixels = reinterpret_cast<const QRgb*>(img.scanLine(i));
    for(int j=0; j<width; j++)
      {
      const QRgb& col = linePixels[j];
      row[j*numcomponents] = qRed(col);
      row[j*numcomponents+1] = qGreen(col);
      row[j*numcomponents+2] = qBlue(col);
      if(numcomponents == 4)
        {
        row[j*numcomponents+3] = qAlpha(col);
        }
      }
    }
  return true;
}

bool pqImageUtil::fromImageData(vtkImageData* vtkimage, QImage& img)
{

  if (vtkimage->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    return false;
    }

  int extent[6];
  vtkimage->GetExtent(extent);
  int width = extent[1]-extent[0]+1;
  int height = extent[3]-extent[2]+1;
  int numcomponents = vtkimage->GetNumberOfScalarComponents();
  if(!(numcomponents == 3 || numcomponents == 4))
    {
    return false;
    }
  
  QImage newimg(width, height, QImage::Format_ARGB32);

  for(int i=0; i<height; i++)
    {
    QRgb* bits = reinterpret_cast<QRgb*>(newimg.scanLine(i));
    unsigned char* row;
    row = static_cast<unsigned char*>(
      vtkimage->GetScalarPointer(extent[0], extent[2] + height-i-1, extent[4]));
    for(int j=0; j<width; j++)
      {
      unsigned char* data = &row[j*numcomponents];
      bits[j] = numcomponents == 4 ? 
        qRgba(data[0], data[1], data[2], data[3]) :
        qRgb(data[0], data[1], data[2]);
      }
    }

  img = newimg;
  return true;
}

