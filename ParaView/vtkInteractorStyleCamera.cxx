/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleCamera.cxx
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
#include "vtkInteractorStyleCamera.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkInteractorStyleCamera *vtkInteractorStyleCamera::New() 
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleCamera");
  if(ret)
    {
    return (vtkInteractorStyleCamera*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleCamera;
}


//----------------------------------------------------------------------------
vtkInteractorStyleCamera::vtkInteractorStyleCamera() 
{
  this->MotionFactor = 10.0;
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
vtkInteractorStyleCamera::~vtkInteractorStyleCamera() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  if (this->State == VTK_INTERACTOR_STYLE_CAMERA_ROTATE)
    {
    this->FindPokedCamera(x, y);
    this->RotateXY(x - this->LastPos[0], y - this->LastPos[1]);
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}


//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::RotateXY(int dx, int dy)
{
  double rxf;
  double ryf;
  vtkCamera *cam;
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();
  this->DeltaElevation = -20.0 / size[1];
  this->DeltaAzimuth = -20.0 / size[0];
  
  rxf = (double)dx * this->DeltaAzimuth *  this->MotionFactor;
  ryf = (double)dy * this->DeltaElevation * this->MotionFactor;
  
  cam = this->CurrentRenderer->GetActiveCamera();
  cam->Azimuth(rxf);
  cam->Elevation(ryf);
  cam->OrthogonalizeViewUp();
  this->CurrentRenderer->ResetCameraClippingRange();
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    // get the first light
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }	
  rwi->Render();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnLeftButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  cerr << "**** Button Down\n";
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->State = VTK_INTERACTOR_STYLE_CAMERA_ROTATE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnLeftButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnMiddleButtonDown(int ctrl, int shift, 
						 int x, int y) 
{
  return;
}
//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnMiddleButtonUp(int ctrl, int shift, 
					       int x, int y) 
{
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnRightButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  return;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::OnRightButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);

}












