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
  this->OriginXLabel = vtkKWLabel::New();
  this->OriginYLabel = vtkKWLabel::New();
  this->OriginZLabel = vtkKWLabel::New();
  this->NormalXLabel = vtkKWLabel::New();
  this->NormalYLabel = vtkKWLabel::New();
  this->NormalZLabel = vtkKWLabel::New();
  this->OriginXEntry = vtkKWEntry::New();
  this->OriginYEntry = vtkKWEntry::New();
  this->OriginZEntry = vtkKWEntry::New();
  this->NormalXEntry = vtkKWEntry::New();
  this->NormalYEntry = vtkKWEntry::New();
  this->NormalZEntry = vtkKWEntry::New();

  this->PlaneStyleButton = vtkKWPushButton::New();
  
  this->PlaneStyle = vtkInteractorStylePlane::New();
  
  this->Cutter = vtkCutter::New();  
}

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->OriginXLabel->Delete();
  this->OriginXLabel = NULL;
  this->OriginYLabel->Delete();
  this->OriginYLabel = NULL;
  this->OriginZLabel->Delete();
  this->OriginZLabel = NULL;
  this->NormalXLabel->Delete();
  this->NormalXLabel = NULL;
  this->NormalYLabel->Delete();
  this->NormalYLabel = NULL;
  this->NormalZLabel->Delete();
  this->NormalZLabel = NULL;
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
  
  this->OriginXLabel->SetParent(this->OriginFrame->GetFrame());
  this->OriginXLabel->Create(this->Application, "");
  this->OriginXLabel->SetLabel("X: ");
  this->OriginXEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginXEntry->Create(this->Application, "-width 12");
  this->OriginXEntry->SetValue(origin[0], 4);
  this->OriginYLabel->SetParent(this->OriginFrame->GetFrame());
  this->OriginYLabel->Create(this->Application, "");
  this->OriginYLabel->SetLabel("Y: ");
  this->OriginYEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginYEntry->Create(this->Application, "-width 12");
  this->OriginYEntry->SetValue(origin[1], 4);
  this->OriginZLabel->SetParent(this->OriginFrame->GetFrame());
  this->OriginZLabel->Create(this->Application, "");
  this->OriginZLabel->SetLabel("Z: ");
  this->OriginZEntry->SetParent(this->OriginFrame->GetFrame());
  this->OriginZEntry->Create(this->Application, "-width 12");
  this->OriginZEntry->SetValue(origin[2], 4);
  this->Script("pack %s %s %s %s %s %s -side left -fill x",
	       this->OriginXLabel->GetWidgetName(),
	       this->OriginXEntry->GetWidgetName(),
	       this->OriginYLabel->GetWidgetName(),
	       this->OriginYEntry->GetWidgetName(),
	       this->OriginZLabel->GetWidgetName(),
	       this->OriginZEntry->GetWidgetName());
  
  this->NormalXLabel->SetParent(this->NormalFrame->GetFrame());
  this->NormalXLabel->Create(this->Application, "");
  this->NormalXLabel->SetLabel("X: ");
  this->NormalXEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalXEntry->Create(this->Application, "-width 12");
  this->NormalXEntry->SetValue(normal[0], 4);
  this->NormalYLabel->SetParent(this->NormalFrame->GetFrame());
  this->NormalYLabel->Create(this->Application, "");
  this->NormalYLabel->SetLabel("Y: ");
  this->NormalYEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalYEntry->Create(this->Application, "-width 12");
  this->NormalYEntry->SetValue(normal[1], 4);
  this->NormalZLabel->SetParent(this->NormalFrame->GetFrame());
  this->NormalZLabel->Create(this->Application, "");
  this->NormalZLabel->SetLabel("Z: ");
  this->NormalZEntry->SetParent(this->NormalFrame->GetFrame());
  this->NormalZEntry->Create(this->Application, "-width 12");
  this->NormalZEntry->SetValue(normal[2], 4);
  this->Script("pack %s %s %s %s %s %s -side left -fill x",
	       this->NormalXLabel->GetWidgetName(),
	       this->NormalXEntry->GetWidgetName(),
	       this->NormalYLabel->GetWidgetName(),
	       this->NormalYEntry->GetWidgetName(),
	       this->NormalZLabel->GetWidgetName(),
	       this->NormalZEntry->GetWidgetName());
  
  this->PlaneStyleButton->SetParent(this->GetWindow()->GetToolbar());
  this->PlaneStyleButton->Create(this->Application, "");
  this->PlaneStyleButton->SetLabel("Plane");
  this->PlaneStyleButton->SetCommand(this, "UsePlaneStyle");
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
	       this->PlaneStyleButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVCutter::UsePlaneStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->PlaneStyle);
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
void vtkPVCutter::SetOutput(vtkPVPolyData *pvd)
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
vtkPVPolyData *vtkPVCutter::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
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
  me->GetPlaneStyle()->GetPlane()->GetOrigin(orig);
  me->GetPlaneStyle()->GetPlane()->GetNormal(norm);
  
  me->GetOriginXEntry()->SetValue(orig[0], 4);
  me->GetOriginYEntry()->SetValue(orig[1], 4);
  me->GetOriginZEntry()->SetValue(orig[2], 4);
  me->GetNormalXEntry()->SetValue(norm[0], 4);
  me->GetNormalYEntry()->SetValue(norm[1], 4);
  me->GetNormalZEntry()->SetValue(norm[2], 4);
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
    this->SetOutput(pvd);
    a = this->GetInput()->GetAssignment();
    pvd->SetAssignment(a);
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
//    this->GetPlaneStyle()->GetCrossHair()->SetModelBounds(this->GetInput()->GetData()->GetBounds());
    }
  window->GetMainView()->SetSelectedComposite(this);
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
