/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImageExtent.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInteractorStyleImageExtent.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
vtkInteractorStyleImageExtent *vtkInteractorStyleImageExtent::New() 
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleImageExtent");
  if(ret)
    {
    return (vtkInteractorStyleImageExtent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleImageExtent;
}

//----------------------------------------------------------------------------
vtkInteractorStyleImageExtent::vtkInteractorStyleImageExtent() 
{
  this->ImageData = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorStyleImageExtent::~vtkInteractorStyleImageExtent() 
{
  this->SetImageData(NULL);
}


//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::SetImageData(vtkImageData *image)
{
  if (image == this->ImageData)
    {
    return;
    }
  if (this->ImageData)
    {
    this->ImageData->UnRegister(this);
    this->ImageData = NULL;
    }
  if (image)
    {
    image->Register(this);
    this->ImageData = image;
    // This will clip the extent with the image whole extent.
    this->SetExtent(this->Extent);
    }
}


//--------------------------------------------------------------------------
int *vtkInteractorStyleImageExtent::GetWholeExtent() 
{
  if (this->ImageData == NULL)
    {
    return NULL;
    }

  this->ImageData->UpdateInformation();
  return this->ImageData->GetWholeExtent();
}


//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleImageExtent::GetWorldSpot(int spotId, float spot[3]) 
{
  int ix, iy, iz;
  float *origin;
  float *spacing;
  int *ext;
  
  if (this->ImageData == NULL)
    {
    spot[0] = spot[1] = spot[2] = 0.0;
    return;
    }

  this->ImageData->UpdateInformation();

  // Decode the corner from the spot id.
  if (spotId >= 4)
    {
    iz = this->Extent[5];
    spotId -= 4;
    }
  else
    {
    iz = this->Extent[4];
    }
  if (spotId >= 2)
    {
    iy = this->Extent[3];
    spotId -= 2;
    }
  else
    {
    iy = this->Extent[2];
    }
  if (spotId >= 1)
    {
    ix = this->Extent[1];
    }
  else
    {
    ix = this->Extent[0];
    }

  // Make sure the spot is in bounds.
  ext = this->ImageData->GetWholeExtent();
  if (ix < ext[0])
    {
    ix = ext[0];
    }
  if (ix > ext[1])
    {
    ix = ext[1];
    }

  if (iy < ext[2])
    {
    iy = ext[2];
    }
  if (iy > ext[3])
    {
    iy = ext[3];
    }

  if (iz < ext[4])
    {
    iz = ext[4];
    }
  if (iz > ext[5])
    {
    iz = ext[5];
    }

  origin = this->ImageData->GetOrigin();
  spacing = this->ImageData->GetSpacing();
  spot[0] = origin[0] + (float)(ix) * spacing[0];
  spot[1] = origin[1] + (float)(iy) * spacing[1];
  spot[2] = origin[2] + (float)(iz) * spacing[2];
}

//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleImageExtent::GetSpotAxes(int spotId, double *v0, 
                                           double *v1, double *v2) 
{
  float *spacing;

  if (this->ImageData == NULL)
    {
    vtkErrorMacro("No image.");
    return;
    }
  
  spotId = spotId;

  this->ImageData->UpdateInformation();
  spacing = this->ImageData->GetSpacing();

  v0[0] = spacing[0];
  v0[1] = v0[2] = 0.0;
  v1[1] = spacing[1];
  v1[0] = v1[2] = 0.0;
  v2[2] = spacing[2];
  v2[0] = v2[1] = 0.0;

}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyleExtent::PrintSelf(os,indent);

  os << indent << "ImageData: (" << this->ImageData << ")" << endl;

}
