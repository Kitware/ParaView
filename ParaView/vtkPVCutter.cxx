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
void PlaneCallback(void *arg)
{
  vtkPVCutter *me = (vtkPVCutter*)arg;
  float orig[3], norm[3];
  vtkPVApplication *pvApp = me->GetPVApplication();
  
  me->GetPlaneStyle()->GetPlane()->GetOrigin(orig);
  me->GetPlaneStyle()->GetPlane()->GetNormal(norm);
  
  me->SetOrigin(orig[0], orig[1], orig[2]);
  me->SetNormal(norm[0], norm[1], norm[2]);
  //pvApp->BroadcastScript("%s SetOrigin %f %f %f",
//			   me->GetTclName(), orig[0], orig[1], orig[2]);
  //   pvApp->BroadcastScript("%s SetNormal %f %f %f",
//			   me->GetTclName(), norm[0], norm[1], norm[2]);

//    me->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
vtkPVCutter::vtkPVCutter()
{
  this->CommandFunction = vtkPVCutterCommand;
  
  this->ShowCrosshairButton = vtkKWCheckButton::New();
  this->ShowCrosshairButton->SetParent(this->Properties);

  this->PlaneStyleButton = vtkKWPushButton::New();  
  this->PlaneStyle = vtkInteractorStylePlane::New();
  this->PlaneStyle->SetCallbackMethod(PlaneCallback, this);
  this->PlaneStyleUsed = 0;
  this->PlaneStyleCreated = 0;
  
  vtkCutter *cutter = vtkCutter::New(); 
  cutter->SetCutFunction(this->PlaneStyle->GetPlane());
  this->SetVTKSource(cutter);
  cutter->Delete();
}

//----------------------------------------------------------------------------
vtkPVCutter::~vtkPVCutter()
{ 
  this->ShowCrosshairButton->Delete();
  this->ShowCrosshairButton = NULL;
  
  this->PlaneStyleButton->Delete();
  this->PlaneStyleButton = NULL;
  
  this->PlaneStyle->Delete();
  this->PlaneStyle = NULL;
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
  
  this->vtkPVDataSetToPolyDataFilter::CreateProperties();

  this->GetPlaneStyle()->GetPlane()->GetOrigin(origin);
  this->GetPlaneStyle()->GetPlane()->GetNormal(normal);  
  
  this->AddVector3Entry("Origin:", "X","Y","Z", "SetOrigin","GetOrigin",this);
  this->AddVector3Entry("Normal:", "X","Y","Z", "SetNormal","GetNormal",this);

  this->UpdateParameterWidgets();
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
void vtkPVCutter::SetOrigin(float x, float y, float z)
{  
  this->PlaneStyle->GetPlane()->SetOrigin(x, y, z);
  this->Origin[0] = x;
  this->Origin[1] = y;
  this->Origin[2] = z;
}
//----------------------------------------------------------------------------
void vtkPVCutter::SetNormal(float x, float y, float z)
{  
  this->PlaneStyle->GetPlane()->SetNormal(x, y, z);
  this->Normal[0] = x;
  this->Normal[1] = y;
  this->Normal[2] = z;
}

//----------------------------------------------------------------------------
void vtkPVCutter::Select(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Select(view);
  
  if (!this->PlaneStyleCreated)
    {
    this->PlaneStyleCreated = 1;
    this->PlaneStyleButton->SetParent(this->GetWindow()->GetToolbar());
    this->PlaneStyleButton->Create(this->Application, "");
    this->PlaneStyleButton->SetLabel("Plane");
    this->PlaneStyleButton->SetCommand(this, "UsePlaneStyle");
    //this->Script("%s configure -state disabled",
		 //this->PlaneStyleButton->GetWidgetName());
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
