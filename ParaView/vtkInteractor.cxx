/*=========================================================================
  
  Program:   ParaView
  Module:    vtkInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include <math.h>
#include "vtkInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkInteractor::vtkInteractor()
{
  this->Renderer = NULL;
  this->Transform = vtkTransform::New();
  this->Transform->PostMultiply();

  this->CameraXAxis[0] = 1.0;
  this->CameraXAxis[1] = 0.0;
  this->CameraXAxis[2] = 0.0;

  this->CameraYAxis[0] = 0.0;
  this->CameraYAxis[1] = 1.0;
  this->CameraYAxis[2] = 0.0;

  this->CameraZAxis[0] = 0.0;
  this->CameraZAxis[1] = 0.0;
  this->CameraZAxis[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkInteractor::~vtkInteractor()
{
  this->SetRenderer(NULL);
  this->Transform->Delete();
  // this->RangeFinder->Delete();
}


//----------------------------------------------------------------------------
void vtkInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  os << indent << "Render: (" << this->Renderer << ")\n";
  os << indent << "CameraXAxis: " << this->CameraXAxis[0] << ", "
     << this->CameraXAxis[1] << ", " << this->CameraXAxis[2] << endl;
  os << indent << "CameraYAxis: " << this->CameraYAxis[0] << ", "
     << this->CameraYAxis[1] << ", " << this->CameraYAxis[2] << endl;
  os << indent << "CameraZAxis: " << this->CameraZAxis[0] << ", "
     << this->CameraZAxis[1] << ", " << this->CameraZAxis[2] << endl;
  os << indent << "Transform: " << endl;
  indent = indent.GetNextIndent();
  this->Transform->PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInteractor::SetRenderer(vtkRenderer *ren)
{
  if (this->Renderer == ren)
    {
    return;
    }
  if (this->Renderer != NULL)
    {
    this->Renderer->UnRegister(this);
    this->Renderer = NULL;
    }
  if (ren != NULL)
    {
    this->Renderer = ren;
    ren->Register(this);
    }
  this->Modified();
}


//----------------------------------------------------------------------------
// This is for interactors that rotate relative 
// to the cameras coordinate system.
void vtkInteractor::ComputeCameraAxes()
{
  vtkCamera *cam;

  if (this->Renderer == NULL)
    {
    return;
    }
  cam = this->Renderer->GetActiveCamera();

  cam->OrthogonalizeViewUp();
  cam->GetViewUp(this->CameraYAxis);
  
  cam->GetDirectionOfProjection(this->CameraZAxis);

  // I do not know if this is actually used, but this was originally
  // set to the ViewPlaneNormal.
  this->CameraZAxis[0] = - this->CameraZAxis[0];
  this->CameraZAxis[1] = - this->CameraZAxis[1];
  this->CameraZAxis[2] = - this->CameraZAxis[2];  

  vtkMath::Cross(this->CameraYAxis, this->CameraZAxis, this->CameraXAxis);
}  



//----------------------------------------------------------------------------
// create a basis, convert to view, and find magnitude.
float vtkInteractor::GetScaleAtPoint(float *inPt)
{
  // there should be an easier way of doing this.
  float p0[4], p1[4], p2[4], p3[4], scale;
  // set up a basis
  p0[0] = inPt[0]; 
  p0[1] = inPt[1]; 
  p0[2] = inPt[2];
  p0[3] = 1.0;
  p1[0] = p0[0]+1;  p1[1] = p0[1];  p1[2] = p0[2];  p1[3] = 1.0;
  p2[0] = p0[0];  p2[1] = p0[1]+1;  p2[2] = p0[2];  p2[3] = 1.0;
  p3[0] = p0[0];  p3[1] = p0[1];  p3[2] = p0[2]+1;  p3[3] = 1.0;
  // Convert to View
  this->Renderer->SetWorldPoint(p0);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p0);
  this->Renderer->SetWorldPoint(p1);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p1);
  this->Renderer->SetWorldPoint(p2);
  this->Renderer->WorldToView(); 
  this->Renderer->GetViewPoint(p2);
  this->Renderer->SetWorldPoint(p3);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p3);
  // now compute a scale factor
  p1[0] -= p0[0];  p1[1] -= p0[1];  
  p2[0] -= p0[0];  p2[1] -= p0[1];  
  p3[0] -= p0[0];  p3[1] -= p0[1];
  scale = p1[0]*p1[0] + p1[1]*p1[1] 
        + p2[0]*p2[0] + p2[1]*p2[1] 
        + p3[0]*p3[0] + p3[1]*p3[1];
  scale = sqrt(scale);

  return scale;
}

//============================================================================
// putting some of this here until I find a better place.


//----------------------------------------------------------------------------
// Just use a cheap and dirty interpolate for now.
// This does not work for singular cases of 180 degree rotation,
// and does not work well for rotations over 90 degrees.
void vtkInteractor::InterpolateCamera(vtkCamera *cam1, vtkCamera *cam2, 
                                      float k, vtkCamera *camOut)
{
  double k1, k2;
  double *v1, *v2, vOut[3];

  k1 = (1.0 - k);
  k2 = k;

  v1 = cam1->GetClippingRange();
  v2 = cam2->GetClippingRange();
  vOut[0] = k1*v1[0] + k2*v2[0];
  vOut[1] = k1*v1[1] + k2*v2[1];
  //camOut->SetClippingRange(vOut);

  v1 = cam1->GetFocalPoint();
  v2 = cam2->GetFocalPoint();
  vOut[0] = k1*v1[0] + k2*v2[0];
  vOut[1] = k1*v1[1] + k2*v2[1];
  vOut[2] = k1*v1[2] + k2*v2[2];
  camOut->SetFocalPoint(vOut);

  v1 = cam1->GetPosition();
  v2 = cam2->GetPosition();
  vOut[0] = k1*v1[0] + k2*v2[0];
  vOut[1] = k1*v1[1] + k2*v2[1];
  vOut[2] = k1*v1[2] + k2*v2[2];
  camOut->SetPosition(vOut);

  v1 = cam1->GetViewUp();
  v2 = cam2->GetViewUp();
  vOut[0] = k1*v1[0] + k2*v2[0];
  vOut[1] = k1*v1[1] + k2*v2[1];
  vOut[2] = k1*v1[2] + k2*v2[2];
  // normalize
  k1 = vOut[0]*vOut[0] + vOut[1]*vOut[1] + vOut[2]*vOut[2];
  if (k1 <= 0.0)
    {
    vtkErrorMacro("Improve camera interpolate method.");
    return;
    }
  k1 = 1.0 / sqrt(k1);
  vOut[0] *= k1;
  vOut[1] *= k1;
  vOut[2] *= k1;
  camOut->SetViewUp(vOut);

}

