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
  this->ConstrainSpheresOff();
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
  int corner[3];
  
  if (this->ImageData == NULL)
    {
    spot[0] = spot[1] = spot[2] = 0.0;
    return;
    }

  this->ImageData->UpdateInformation();
  ext = this->ImageData->GetWholeExtent();

  // Decode the corner from the spot id.
  if (spotId >= 4)
    {
    iz = this->Extent[5];
    corner[2] = ext[5];
    spotId -= 4;
    }
  else
    {
    iz = this->Extent[4];
    corner[2] = ext[4];
    }
  if (spotId >= 2)
    {
    iy = this->Extent[3];
    corner[1] = ext[3];
    spotId -= 2;
    }
  else
    {
    iy = this->Extent[2];
    corner[1] = ext[2];
    }
  if (spotId >= 1)
    {
    ix = this->Extent[1];
    corner[0] = ext[1];
    }
  else
    {
    ix = this->Extent[0];
    corner[0] = ext[0];
    }

  // Make sure the spot is in bounds.
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

  if (this->ConstrainSpheres && this->Constraint0)
    {
    spot[0] = origin[0] + (float)(ix) * spacing[0];
    spot[1] = (float)(corner[1]) * spacing[1];
    spot[2] = (float)(corner[2]) * spacing[2];
    }
  else if (this->ConstrainSpheres && this->Constraint1)
    {
    spot[0] = (float)(corner[0]) * spacing[0];
    spot[1] = origin[1] + (float)(iy) * spacing[1];
    spot[2] = (float)(corner[2]) * spacing[2];
    }
  else if (this->ConstrainSpheres && this->Constraint2)
    {
    spot[0] = float(corner[0]) * spacing[0];
    spot[1] = float(corner[1]) * spacing[1];
    spot[2] = origin[2] + (float)(iz) * spacing[2];
    }
  else
    {
    spot[0] = origin[0] + (float)(ix) * spacing[0];
    spot[1] = origin[1] + (float)(iy) * spacing[1];
    spot[2] = origin[2] + (float)(iz) * spacing[2];
    }
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
void vtkInteractorStyleImageExtent::OnMouseMove(int ctrl, int shift, int x,
						int y)
{
  if (this->Button == -1)
    {
    // HandleIndicator sets the CurrentSpotId and computes the size of the
    // corresponding sphere.
    this->HandleIndicator(x, y);
    }

  if (this->Button == 1 && this->CurrentSpotId != -1)
    {
    if (this->Constraint0 || this->Constraint1 || this->Constraint2)
      {
      this->TranslateZ(x - this->LastPos[0], y - this->LastPos[1]);
      }
    else
      {
      this->SetCallbackType("InteractiveRender");
      (*this->CallbackMethod)(this);
      }
    }
  else
    {
    this->vtkInteractorStyleExtent::OnMouseMove(ctrl, shift, x, y);
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::TranslateZ(int vtkNotUsed(dx), int dy)
{
  int *ext = this->GetWholeExtent();
  int *size = this->CurrentRenderer->GetSize();
  
  if (this->TranslateAxis == 1)
    {
    this->Extent[0] += (float)(dy) / (float)(size[1]) *
      (float)(ext[1] - ext[0]);
    if (this->Extent[0] > ext[1])
      {
      this->Extent[0] = ext[1];
      }
    else if (this->Extent[0] < ext[0])
      {
      this->Extent[0] = ext[0];
      }
    this->Extent[1] = this->Extent[0];
    }
  else if (this->TranslateAxis == 2)
    {
    this->Extent[2] += (float)(dy) / (float)(size[1]) *
      (float)(ext[3] - ext[2]);
    if (this->Extent[2] > ext[3])
      {
      this->Extent[2] = ext[3];
      }
    else if (this->Extent[2] < ext[2])
      {
      this->Extent[2] = ext[2];
      }
    this->Extent[3] = this->Extent[2];
    }
  else if (this->TranslateAxis == 3)
    {
    this->Extent[4] += (float)(dy) / (float)(size[1]) *
      (float)(ext[5] - ext[4]);
    if (this->Extent[4] > ext[5])
      {
      this->Extent[4] = ext[5];
      }
    else if (this->Extent[4] < ext[4])
      {
      this->Extent[4] = ext[4];
      }
    this->Extent[5] = this->Extent[4];
    }
  else if (this->TranslateAxis == -1)
    {
    this->Extent[0] -= (float)(dy) / (float)(size[1]) *
      (float)(ext[1] - ext[0]);
    if (this->Extent[0] > ext[1])
      {
      this->Extent[0] = ext[1];
      }
    else if (this->Extent[0] < ext[0])
      {
      this->Extent[0] = ext[0];
      }
    this->Extent[1] = this->Extent[0];
    }
  else if (this->TranslateAxis == -2)
    {
    this->Extent[2] -= (float)(dy) / (float)(size[1]) *
      (float)(ext[3] - ext[2]);
    if (this->Extent[2] > ext[3])
      {
      this->Extent[2] = ext[3];
      }
    else if (this->Extent[2] < ext[2])
      {
      this->Extent[2] = ext[2];
      }
    this->Extent[3] = this->Extent[2];
    }
  else if (this->TranslateAxis == -3)
    {
    this->Extent[4] -= (float)(dy) / (float)(size[1]) *
      (float)(ext[5] - ext[4]);
    if (this->Extent[4] > ext[5])
      {
      this->Extent[4] = ext[5];
      }
    else if (this->Extent[4] < ext[4])
      {
      this->Extent[4] = ext[4];
      }
    this->Extent[5] = this->Extent[4];
    }

  this->SetCallbackType("InteractiveRender");
  (*this->CallbackMethod)(this->CallbackMethodArg);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::GetTranslateAxis()
{
  double axis0[3], axis1[3], axis2[3], negAxis0[3], negAxis1[3], negAxis2[3];
  double vpn[3];
  double dotX, dotY, dotZ, dotNegX, dotNegY, dotNegZ, magnitude;
  int i;
  
  // Get the three axes.
  this->GetSpotAxes(this->CurrentSpotId, axis0, axis1, axis2);
  for (i = 0; i < 3; i++)
    {
    negAxis0[i] = -axis0[i];
    negAxis1[i] = -axis1[i];
    negAxis2[i] = -axis2[i];
    }
  this->CurrentRenderer->GetActiveCamera()->GetViewPlaneNormal(vpn);
  
  dotX = vtkMath::Dot(axis0, vpn);
  dotY = vtkMath::Dot(axis1, vpn);
  dotZ = vtkMath::Dot(axis2, vpn);
  dotNegX = vtkMath::Dot(negAxis0, vpn);
  dotNegY = vtkMath::Dot(negAxis1, vpn);
  dotNegZ = vtkMath::Dot(negAxis2, vpn);
  
  magnitude = dotX;
  this->TranslateAxis = 1;

  if (dotY > magnitude)
    {
    magnitude = dotY;
    this->TranslateAxis = 2;
    }
  if (dotZ > magnitude)
    {
    magnitude = dotZ;
    this->TranslateAxis = 3;
    }
  if (dotNegX > magnitude)
    {
    magnitude = dotNegX;
    this->TranslateAxis = -1;
    }
  if (dotNegY > magnitude)
    {
    magnitude = dotNegY;
    this->TranslateAxis = -2;
    }
  if (dotNegZ > magnitude)
    {
    magnitude = dotNegZ;
    this->TranslateAxis = -3;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::OnMiddleButtonDown(int ctrl, int shift, 
						       int X, int Y) 
{
  this->UpdateInternalState(ctrl, shift, X, Y);

  this->FindPokedCamera(X, Y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->Button = 1;
  this->HandleIndicator(X, Y);
  this->GetTranslateAxis();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::OnMiddleButtonUp(int ctrl, int shift, 
						     int X, int Y) 
{
  //
  this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->HasObserver(vtkCommand::MiddleButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
    }
  else 
    {
    }
  
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyleExtent::PrintSelf(os,indent);

  os << indent << "ImageData: (" << this->ImageData << ")" << endl;

}
