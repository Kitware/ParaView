/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraControl.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
#include "vtkPVTraceHelper.h"

vtkStandardNewMacro(vtkPVCameraControl);
vtkCxxRevisionMacro(vtkPVCameraControl, "1.8");

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

  if (this->ElevationEntry->GetValueAsFloat() != angle)
    {
    this->ElevationEntry->SetValue(angle);
    }
  
  float *center = this->InteractorStyle->GetCenter();
  cam->OrthogonalizeViewUp();
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
  
  this->GetTraceHelper()->AddEntry("$kw(%s) Elevation %f", this->GetTclName(), angle);
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
  
  if (this->AzimuthEntry->GetValueAsFloat() != angle)
    {
    this->AzimuthEntry->SetValue(angle);
    }
  
  float *center = this->InteractorStyle->GetCenter();
  cam->OrthogonalizeViewUp();
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

  this->GetTraceHelper()->AddEntry("$kw(%s) Azimuth %f", this->GetTclName(), angle);
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

  if (this->RollEntry->GetValueAsFloat() != angle)
    {
    this->RollEntry->SetValue(angle);
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

  this->GetTraceHelper()->AddEntry("$kw(%s) Roll %f", this->GetTclName(), angle);
}

void vtkPVCameraControl::Create(vtkKWApplication *app, const char *)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->ElevationButton->SetParent(this);
  this->ElevationButton->Create(app, 0);
  this->ElevationButton->SetText("Apply Elevation");
  this->ElevationButton->SetWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->ElevationButton->SetCommand(this, "ElevationButtonCallback");

  this->ElevationEntry->SetParent(this);
  this->ElevationEntry->Create(app, 0);
  this->ElevationEntry->SetValue(0);
  this->ElevationEntry->SetWidth(5);
  
  this->ElevationLabel->SetParent(this);
  this->ElevationLabel->Create(app, 0);
  this->ElevationLabel->SetText("degrees");
  
  this->AzimuthButton->SetParent(this);
  this->AzimuthButton->Create(app, 0);
  this->AzimuthButton->SetText("Apply Azimuth");
  this->AzimuthButton->SetWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->AzimuthButton->SetCommand(this, "AzimuthButtonCallback");

  this->AzimuthEntry->SetParent(this);
  this->AzimuthEntry->Create(app, 0);
  this->AzimuthEntry->SetValue(0);
  this->AzimuthEntry->SetWidth(5);
  
  this->AzimuthLabel->SetParent(this);
  this->AzimuthLabel->Create(app, 0);
  this->AzimuthLabel->SetText("degrees");
  
  this->RollButton->SetParent(this);
  this->RollButton->Create(app, 0);
  this->RollButton->SetText("Apply Roll");
  this->RollButton->SetWidth(VTK_PV_CAMERA_CONTROL_LABEL_WIDTH);
  this->RollButton->SetCommand(this, "RollButtonCallback");

  this->RollEntry->SetParent(this);
  this->RollEntry->Create(app, 0);
  this->RollEntry->SetValue(0);
  this->RollEntry->SetWidth(5);
  
  this->RollLabel->SetParent(this);
  this->RollLabel->Create(app, 0);
  this->RollLabel->SetText("degrees");
  
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
