/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraControl.cxx
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
#include "vtkPVCameraControl.h"

#include "vtkCamera.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVCameraControl);
vtkCxxRevisionMacro(vtkPVCameraControl, "1.1");

vtkCxxSetObjectMacro(vtkPVCameraControl, InteractorStyle,
                     vtkPVInteractorStyleCenterOfRotation);

#define VTK_PV_CAMERA_CONTROL_LABEL_WIDTH 30

vtkPVCameraControl::vtkPVCameraControl()
{
  this->InteractorStyle = NULL;
  this->RenderView = NULL;
  
  this->ElevationButton = vtkKWPushButton::New();
  this->ElevationEntry = vtkKWEntry::New();
  this->ElevationLabel = vtkKWLabel::New();
  
  this->AzimuthButton = vtkKWPushButton::New();
  this->AzimuthEntry = vtkKWEntry::New();
  this->AzimuthLabel = vtkKWLabel::New();
  
  this->RollButton = vtkKWPushButton::New();
  this->RollEntry = vtkKWEntry::New();
  this->RollLabel = vtkKWLabel::New();
}

vtkPVCameraControl::~vtkPVCameraControl()
{
  this->SetInteractorStyle(NULL);
  this->SetRenderView(NULL);
  
  this->ElevationButton->Delete();
  this->ElevationEntry->Delete();
  this->ElevationLabel->Delete();
  
  this->AzimuthButton->Delete();
  this->AzimuthEntry->Delete();
  this->AzimuthLabel->Delete();
  
  this->RollButton->Delete();
  this->RollEntry->Delete();
  this->RollLabel->Delete();
}

void vtkPVCameraControl::Elevation(double angle)
{
  if (!this->InteractorStyle || !this->RenderView)
    {
    return;
    }
  
  vtkCamera *cam = this->RenderView->GetRenderer()->GetActiveCamera();
  if (!cam)
    {
    return;
    }
  
  float *center = this->InteractorStyle->GetCenter();
  cam->OrthogonalizeViewUp();
  double *fp = cam->GetFocalPoint();
  double *pos = cam->GetPosition();
  double *vup = cam->GetViewUp();
  double v2[3];
  
  vtkMath::Cross(cam->GetDirectionOfProjection(), vup, v2);
  
  vtkTransform *xform = vtkTransform::New();
  xform->Identity();
  xform->Translate(center[0], center[1], center[2]);
  xform->RotateWXYZ(angle, v2);
  xform->Translate(-center[0], -center[1], -center[2]);
  
  cam->ApplyTransform(xform);
  cam->OrthogonalizeViewUp();
  
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  
  this->RenderView->Render();
  
  xform->Delete();
}

void vtkPVCameraControl::Azimuth(double angle)
{
  if (!this->InteractorStyle || !this->RenderView)
    {
    return;
    }
  
  vtkCamera *cam = this->RenderView->GetRenderer()->GetActiveCamera();
  if (!cam)
    {
    return;
    }
  
  float *center = this->InteractorStyle->GetCenter();
  cam->OrthogonalizeViewUp();
  double *fp = cam->GetFocalPoint();
  double *pos = cam->GetPosition();
  double *vup = cam->GetViewUp();
  
  vtkTransform *xform = vtkTransform::New();
  xform->Identity();
  xform->Translate(center[0], center[1], center[2]);
  xform->RotateWXYZ(angle, vup);
  xform->Translate(-center[0], -center[1], -center[2]);
  
  cam->ApplyTransform(xform);
  cam->OrthogonalizeViewUp();
  
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  
  this->RenderView->Render();
  
  xform->Delete();
}

void vtkPVCameraControl::Roll(double angle)
{
  if (!this->InteractorStyle || !this->RenderView)
    {
    return;
    }
  
  vtkCamera *cam = this->RenderView->GetRenderer()->GetActiveCamera();
  if (!cam)
    {
    return;
    }
  
  float *center = this->InteractorStyle->GetCenter();
  cam->OrthogonalizeViewUp();
  double *fp = cam->GetFocalPoint();
  double *pos = cam->GetPosition();
  double axis[3];
  
  axis[0] = fp[0] - pos[0];
  axis[1] = fp[1] - pos[1];
  axis[2] = fp[2] - pos[2];
  
  vtkTransform *xform = vtkTransform::New();
  xform->Identity();
  xform->Translate(center[0], center[1], center[2]);
  xform->RotateWXYZ(angle, axis);
  xform->Translate(-center[0], -center[1], -center[2]);
  
  cam->ApplyTransform(xform);
  cam->OrthogonalizeViewUp();
  
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  
  this->RenderView->Render();
  
  xform->Delete();
}

void vtkPVCameraControl::Create(vtkKWApplication *app, const char *)
{
  if (this->Application)
    {
    vtkErrorMacro("vtkPVCameraControl has already been created");
    return;
    }
  
  this->SetApplication(app);
  
  this->Script("frame %s -bd 0", this->GetWidgetName());
  
  this->ElevationButton->SetParent(this);
  this->ElevationButton->Create(app, 0);
  this->ElevationButton->SetLabel("Apply Elevation");
  this->ElevationButton->SetLabelWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->ElevationButton->SetCommand(this, "ElevationButtonCallback");

  this->ElevationEntry->SetParent(this);
  this->ElevationEntry->Create(app, 0);
  this->ElevationEntry->SetValue(0);
  this->ElevationEntry->SetWidth(5);
  
  this->ElevationLabel->SetParent(this);
  this->ElevationLabel->Create(app, 0);
  this->ElevationLabel->SetLabel("degrees");
  
  this->AzimuthButton->SetParent(this);
  this->AzimuthButton->Create(app, 0);
  this->AzimuthButton->SetLabel("Apply Azimuth");
  this->AzimuthButton->SetLabelWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->AzimuthButton->SetCommand(this, "AzimuthButtonCallback");

  this->AzimuthEntry->SetParent(this);
  this->AzimuthEntry->Create(app, 0);
  this->AzimuthEntry->SetValue(0);
  this->AzimuthEntry->SetWidth(5);
  
  this->AzimuthLabel->SetParent(this);
  this->AzimuthLabel->Create(app, 0);
  this->AzimuthLabel->SetLabel("degrees");
  
  this->RollButton->SetParent(this);
  this->RollButton->Create(app, 0);
  this->RollButton->SetLabel("Apply Roll");
  this->RollButton->SetLabelWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->RollButton->SetCommand(this, "RollButtonCallback");

  this->RollEntry->SetParent(this);
  this->RollEntry->Create(app, 0);
  this->RollEntry->SetValue(0);
  this->RollEntry->SetWidth(5);
  
  this->RollLabel->SetParent(this);
  this->RollLabel->Create(app, 0);
  this->RollLabel->SetLabel("degrees");
  
  this->Script("grid %s -row 0 -column 0 -padx 3",
               this->ElevationButton->GetWidgetName());
  this->Script("grid %s -row 0 -column 1",
               this->ElevationEntry->GetWidgetName());
  this->Script("grid %s -row 0 -column 2",
               this->ElevationLabel->GetWidgetName());
  this->Script("grid %s -row 1 -column 0 -padx 3",
               this->AzimuthButton->GetWidgetName());
  this->Script("grid %s -row 1 -column 1",
               this->AzimuthEntry->GetWidgetName());
  this->Script("grid %s -row 1 -column 2",
               this->AzimuthLabel->GetWidgetName());
  this->Script("grid %s -row 2 -column 0 -padx 3",
               this->RollButton->GetWidgetName());
  this->Script("grid %s -row 2 -column 1",
               this->RollEntry->GetWidgetName());
  this->Script("grid %s -row 2 -column 2",
               this->RollLabel->GetWidgetName());
}

void vtkPVCameraControl::ElevationButtonCallback()
{
  this->Elevation(this->ElevationEntry->GetValueAsFloat());
}

void vtkPVCameraControl::AzimuthButtonCallback()
{
  this->Azimuth(this->AzimuthEntry->GetValueAsFloat());
}

void vtkPVCameraControl::RollButtonCallback()
{
  this->Roll(this->RollEntry->GetValueAsFloat());
}

void vtkPVCameraControl::SetRenderView(vtkPVRenderView *view)
{
  // avoid circular referencing
  this->RenderView = view;
}

void vtkPVCameraControl::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
