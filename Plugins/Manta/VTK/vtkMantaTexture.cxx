/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaTexture.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkManta.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaTexture.h"
#include "vtkMantaRenderWindow.h"

#include "vtkHomogeneousTransform.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTransform.h"

#include <Image/SimpleImage.h>
#include <Model/Textures/ImageTexture.h>

#include <math.h>

vtkCxxRevisionMacro(vtkMantaTexture, "1.2");
vtkStandardNewMacro(vtkMantaTexture);

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkMantaTexture::vtkMantaTexture()
{
  cerr << "CREATE MANTA TEXTURE " << this << endl;
  this->RenderWindow = 0;
}

//----------------------------------------------------------------------------
vtkMantaTexture::~vtkMantaTexture()
{
  cerr << "DESTROY MANTA TEXTURE " << this << endl;

  if (this->RenderWindow)
    {
    this->ReleaseGraphicsResources(this->RenderWindow);
    }
  this->RenderWindow = NULL;
}

//-----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkMantaTexture::ReleaseGraphicsResources(vtkWindow *renWin)
{
  //this->RenderWindow = NULL;
  //this->Modified();
}

//----------------------------------------------------------------------------
void vtkMantaTexture::Load(vtkRenderer *ren)
{
  vtkImageData *input = this->GetInput();

  vtkMantaRenderWindow* renWin =
      vtkMantaRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
    {
    return;
    }

  if (this->GetMTime() > this->LoadTime.GetMTime() ||
      input->GetMTime()> this->LoadTime.GetMTime() ||
      (this->GetLookupTable() && this->GetLookupTable()->GetMTime() > this->LoadTime.GetMTime()) //||
      /*renWin != this->RenderWindow.GetPointer() || renWin->GetContextCreationTime() > this->LoadTime*/)
    {
    int bytesPerPixel;
    int size[3];
    vtkDataArray *scalars;
    unsigned char *dataPtr;
    int xsize, ysize;

    // Get the scalars the user choose to color with.
    scalars = this->GetInputArrayToProcess(0, input);
    // make sure scalars are non null
    if (!scalars)
      {
      vtkErrorMacro(<< "No scalar values found for texture input!");
      return;
      }

    // get some info
    input->GetDimensions(size);

    if (input->GetNumberOfCells() == scalars->GetNumberOfTuples())
      {
      // we are using cell scalars. Adjust image size for cells.
      for (int kk = 0; kk < 3; kk++)
        {
        if (size[kk] > 1)
          {
          size[kk]--;
          }
        }
      }

    bytesPerPixel = scalars->GetNumberOfComponents();

    // make sure using unsigned char data of color scalars type
    if (this->MapColorScalarsThroughLookupTable || scalars->GetDataType()
        != VTK_UNSIGNED_CHAR)
      {
      dataPtr = this->MapScalarsToColors(scalars);
      bytesPerPixel = 4;
      }
    else
      {
      dataPtr = static_cast<vtkUnsignedCharArray *> (scalars)->GetPointer(0);
      }

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it
    // could be any of them, so lets find it
    if (size[0] == 1)
      {
      xsize = size[1];
      ysize = size[2];
      }
    else
      {
      xsize = size[0];
      if (size[1] == 1)
        {
        ysize = size[2];
        }
      else
        {
        ysize = size[1];
        if (size[2] != 1)
          {
          vtkErrorMacro(<< "3D texture maps currently are not supported!");
          return;
          }
        }
      }

    // Create Manta Image from input
    Manta::Image *image = new Manta::SimpleImage<Manta::RGB8Pixel> (false, xsize, ysize);
    Manta::RGB8Pixel *pixels = dynamic_cast<Manta::SimpleImage<Manta::RGB8Pixel> const*>(image)->getRawPixels(0);
    Manta::RGB8Pixel pixel;
    for (unsigned v = 0; v < ysize; v++)
      {
      for (unsigned u = 0; u < xsize; u++)
        {
        unsigned char *color = &dataPtr[(v*xsize+u)*bytesPerPixel];
        pixel.r = color[0];
        pixel.g = color[1];
        pixel.b = color[2];
        pixels[v*xsize + u] = pixel;
        }
      }

    // create Manta texture from the image
    Manta::ImageTexture<Manta::Color> *imgtexture = new Manta::ImageTexture<Manta::Color>(image, false);
    this->MantaTexture = imgtexture;
    imgtexture->setInterpolationMethod(1);
    // Manta image is copied and converted to internal buffer in the texture,
    // delete the image
    delete image;

    this->LoadTime.Modified();
    }

}

//----------------------------------------------------------------------------
void vtkMantaTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


