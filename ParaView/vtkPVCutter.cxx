/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCutter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVCutter.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkKWToolbar.h"

int vtkPVCutterCommand(ClientData cd, Tcl_Interp *interp,
		       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter()
{
  this->CommandFunction = vtkPVCutterCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);

  this->OriginFrame = vtkKWLabeledFrame::New();
  this->OriginFrame->SetParent(this->Properties);
  this->NormalFrame = vtkKWLabeledFrame::New();
  this->NormalFrame->SetParent(this->Properties);
  this->OriginXEntry = vtkKWLabeledEntry::New();
  this->OriginYEntry = vtkKWLabeledEntry::New();
  this->OriginZEntry = vtkKWLabeledEntry::New();
  this->NormalXEntry = vtkKWLabeledEntry::New();
  this->NormalYEntry = vtkKWLabeledEntry::New();
  this->NormalZEntry = vtkKWLabeledEntry::New();
  this->ShowCrosshairButton = vtkKWCheckButton::New();
  this->ShowCrosshairButton->SetParent(this->Properties);

  this->PlaneStyleButton = vtkKWPushButton::New();  
  this->PlaneStyle = vtkInteractorStylePlane::New();
  this->PlaneStyleCreated = 0;
  this->PlaneStyleUsed = 0;
  
  this->Cutter = vtkCutter::New();  
}

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->OriginXEntry->Delete();
  this->OriginXEntry = NULL;
  this->OriginYEntry->Delete();
  this->OriginYEntry = NULL;
  this->OriginZEntry->Delete();
  this->OriginZEntry = NULL;
  this->NormalXEntry->Delete();
  this->NormalXEntry = NULL;
  this->NormalYEntry->Delete();
  this->NormalYEntry = NULL;
  this->NormalZEntry->Delete();
  this->NormalZEntry = NULL;
  this->OriginFrame->Delete();
  this->OriginFrame = NULL;
  this->NormalFrame->Delete();
  this->NormalFrame = NULL;
  this->ShowCrosshairButton->Delete();
  this->ShowCrosshairButton = NULL;
  
  this->PlaneStyleButton->Delete();
  this->PlaneStyleButton = NULL;
  
  this->PlaneStyle->Delete();
  this->PlaneStyle = NULL;
  
  this->Cutter->Delete();
  this->Cutter = NULL;
}

//----------------------------------------------------------------------------
vtkPVCutter* vtkPVCutter::New()
{
  return new vtkPVCutter();
}

//----------------------------------------------------------------------------
void vtkPVCutter::CreateProperties()
{
  float origin[3], normal[3];
  
  this->GetPlaneStyle()->GetPlane()->GetOrigin(origin);
  this->GetPlaneStyle()->GetPlane()->GetNormal(normal);
  
  this->vtkPVSource::CreateProperties();
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Script("pack %s", this->SourceButton->GetWidgetName());

  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "CutterChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  
  this->OriginFrame->Create(this->Application);
  this->OriginFrame->SetLabel("Origin");
  this->NormalFrame->Create(this->Application);
  this->NormalFrame->SetLabel("Normal");
  this->Script("pack %s %s", this->OriginFrame->GetWidgetName(),
	       this->NormalFrame->GetWidgetName());
  
  this->OriginXEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginXEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->OriginXEntry->GetEntry()->GetWidgetName());
  this->OriginXEntry->SetLabel("X: ");
  this->OriginXEntry->SetValue(origin[0], 4);
  this->OriginYEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginYEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->OriginYEntry->GetEntry()->GetWidgetName());
  this->OriginYEntry->SetLabel("Y: ");
  this->OriginYEntry->SetValue(origin[1], 4);
  this->OriginZEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginZEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->OriginZEntry->GetEntry()->GetWidgetName());
  this->OriginZEntry->SetLabel("Z: ");
  this->OriginZEntry->SetValue(origin[2], 4);
  this->Script("pack %s %s %s -side left -fill x",
	       this->OriginXEntry->GetWidgetName(),
	       this->OriginYEntry->GetWidgetName(),
	       this->OriginZEntry->GetWidgetName());
  
  this->NormalXEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalXEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->NormalXEntry->GetEntry()->GetWidgetName());
  this->NormalXEntry->SetLabel("X: ");
  this->NormalXEntry->SetValue(normal[0], 4);
  this->NormalYEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalYEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->NormalYEntry->GetEntry()->GetWidgetName());
  this->NormalYEntry->SetLabel("Y: ");
  this->NormalYEntry->SetValue(normal[1], 4);
  this->NormalZEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalZEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->NormalZEntry->GetEntry()->GetWidgetName());
  this->NormalZEntry->SetLabel("Z: ");
  this->NormalZEntry->SetValue(normal[2], 4);
  this->Script("pack %s %s %s -side left -fill x",
	       this->NormalXEntry->GetWidgetName(),
	       this->NormalYEntry->GetWidgetName(),
	       this->NormalZEntry->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkPVCutter::UsePlaneStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->PlaneStyle);
  if (!this->PlaneStyleUsed)
    {
    this->PlaneStyleUsed = 1;
    this->ShowCrosshairButton->Create(this->Application,
				      "-text ShowCrosshair");
    this->ShowCrosshairButton->SetState(0);
    this->ShowCrosshairButton->SetCommand(this, "CrosshairDisplay");
    this->Script("pack %s", this->ShowCrosshairButton->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVCutter::SetInput(vtkPVData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetCutter()->SetInput(pvData->GetData());
  this->Input = pvData;
}

//----------------------------------------------------------------------------
void vtkPVCutter::SetPVOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvd->GetTclName());
    }  
  
  this->SetPVData(pvd);  
  pvd->SetData(this->Cutter->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVCutter::GetPVOutput()
{
  return vtkPVPolyData::SafeDownCast(this->PVOutput);
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartCutterProgress(void *arg)
{
  vtkPVCutter *me = (vtkPVCutter*)arg;
  me->GetWindow()->SetStatusText("Processing Cutter");
}

//----------------------------------------------------------------------------
void CutterProgress(void *arg)
{
  vtkPVCutter *me = (vtkPVCutter*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetCutter()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndCutterProgress(void *arg)
{
  vtkPVCutter *me = (vtkPVCutter*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void PlaneCallback(void *arg)
{
  vtkPVCutter *me = (vtkPVCutter*)arg;
  float orig[3], norm[3];
  vtkPVApplication *pvApp = me->GetPVApplication();
  
  me->GetPlaneStyle()->GetPlane()->GetOrigin(orig);
  me->GetPlaneStyle()->GetPlane()->GetNormal(norm);
  
  me->GetOriginXEntry()->SetValue(orig[0], 4);
  me->GetOriginYEntry()->SetValue(orig[1], 4);
  me->GetOriginZEntry()->SetValue(orig[2], 4);
  me->GetNormalXEntry()->SetValue(norm[0], 4);
  me->GetNormalYEntry()->SetValue(norm[1], 4);
  me->GetNormalZEntry()->SetValue(norm[2], 4);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetCutPlane %f %f %f %f %f %f",
			   me->GetTclName(), orig[0], orig[1], orig[2],
			   norm[0], norm[1], norm[2]);
    }
  // We don't have to call SetCutPlane in process 0 because it already has the
  // updated plane.
}

//----------------------------------------------------------------------------
void vtkPVCutter::CutterChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;

  this->SetCutPlane(this->OriginXEntry->GetValueAsFloat(),
		    this->OriginYEntry->GetValueAsFloat(),
		    this->OriginZEntry->GetValueAsFloat(),
		    this->NormalXEntry->GetValueAsFloat(),
		    this->NormalYEntry->GetValueAsFloat(),
		    this->NormalZEntry->GetValueAsFloat());
  
  if (this->GetPVData() == NULL)
    { // This is the first time.  Create the data.
    this->GetCutter()->SetStartMethod(StartCutterProgress, this);
    this->GetCutter()->SetProgressMethod(CutterProgress, this);
    this->GetCutter()->SetEndMethod(EndCutterProgress, this);
    this->GetPlaneStyle()->SetCallbackMethod(PlaneCallback, this);
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    this->SetPVOutput(pvd);
    a = this->GetInput()->GetAssignment();
    pvd->SetAssignment(a);
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    window->GetSourceList()->Update();
    }
  window->GetMainView()->SetSelectedComposite(this);
  
  if (!this->PlaneStyleCreated)
    {
    this->PlaneStyleCreated = 1;
    this->Script("%s configure -state normal",
		 this->PlaneStyleButton->GetWidgetName());
    }
  
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVCutter::SetCutPlane(float originX, float originY, float originZ,
			      float normalX, float normalY, float normalZ)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetCutPlane %f %f %f %f %f %f",
			   this->GetTclName(), originX, originY, originZ,
			   normalX, normalY, normalZ);
    }

  this->PlaneStyle->GetPlane()->SetOrigin(originX, originY, originZ);
  this->PlaneStyle->GetPlane()->SetNormal(normalX, normalY, normalZ);
  this->GetCutter()->SetCutFunction(this->PlaneStyle->GetPlane());
}

//----------------------------------------------------------------------------
void vtkPVCutter::GetSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVCutter::Select(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Select(view);
  
  if (!this->PlaneStyleCreated)
    {
    this->PlaneStyleButton->SetParent(this->GetWindow()->GetToolbar());
    this->PlaneStyleButton->Create(this->Application, "");
    this->PlaneStyleButton->SetLabel("Plane");
    this->PlaneStyleButton->SetCommand(this, "UsePlaneStyle");
    this->Script("%s configure -state disabled",
		 this->PlaneStyleButton->GetWidgetName());
    }
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
	       this->PlaneStyleButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVCutter::Deselect(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Deselect(view);
  
  // unpack plane style button and reset interactor style to trackball camera
  this->Script("pack forget %s", this->PlaneStyleButton->GetWidgetName());
  this->GetWindow()->UseCameraStyle();
  this->GetPlaneStyle()->HideCrosshair();
}

//----------------------------------------------------------------------------
void vtkPVCutter::CrosshairDisplay()
{
  if (this->ShowCrosshairButton->GetState())
    {
    this->GetPlaneStyle()->ShowCrosshair();
    }
  else
    {
    this->GetPlaneStyle()->HideCrosshair();
    }
}
