/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCenterOfRotation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWApplication.h"
#include "vtkKWCenterOfRotation.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkKWEntry.h"
#include "vtkKWCheckButton.h"
#include "vtkPVWorldPointPicker.h"
#include "vtkActorCollection.h"
#include "vtkPropPicker.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkTreeComposite.h"


int vtkKWCenterOfRotationCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCenterOfRotation::vtkKWCenterOfRotation()
{
  this->CommandFunction = vtkKWCenterOfRotationCommand;

  this->CameraInteractor = NULL;
  this->ParentInteractor = NULL;

  this->PickButton = vtkKWWidget::New(); 
  this->PickButton->SetParent(this); 

  this->ResetButton = vtkKWWidget::New(); 
  this->ResetButton->SetParent(this);

  this->OpenButton = vtkKWWidget::New(); 
  this->OpenButton->SetParent(this);

  this->XLabel = vtkKWWidget::New();
  this->XLabel->SetParent(this);
  this->XEntry = vtkKWEntry::New();
  this->XEntry->SetParent(this);

  this->YLabel = vtkKWWidget::New();
  this->YLabel->SetParent(this);
  this->YEntry = vtkKWEntry::New();
  this->YEntry->SetParent(this);

  this->ZLabel = vtkKWWidget::New();
  this->ZLabel->SetParent(this);
  this->ZEntry = vtkKWEntry::New();
  this->ZEntry->SetParent(this);

  this->CloseButton = vtkKWWidget::New(); 
  this->CloseButton->SetParent(this);

  this->RenderView = NULL; 

  // For interactively choosing the center of rotation.
  this->Picker = vtkPVWorldPointPicker::New();

  // Display of the center of rotation
  this->CenterSource = vtkAxes::New();
  this->CenterMapper = vtkPolyDataMapper::New();
  this->CenterActor = vtkActor::New();
  this->CenterActor->PickableOff();
  this->CenterSource->SymmetricOn();
  this->CenterSource->ComputeNormalsOff();
  this->CenterMapper->SetInput(this->CenterSource->GetOutput());
  this->CenterActor->SetMapper(this->CenterMapper);
  //this->CenterActor->GetProperty()->SetAmbient(0.6);
  this->CenterActor->VisibilityOff();

  this->DefaultFlag = 1;
}

//----------------------------------------------------------------------------
vtkKWCenterOfRotation::~vtkKWCenterOfRotation()
{
  this->PrepareForDelete();
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::PrepareForDelete()
{
  this->SetCameraInteractor(NULL);
  this->SetParentInteractor(NULL);

  if (this->PickButton)
    {
    this->PickButton->Delete();
    this->PickButton = NULL;
    }

  if (this->ResetButton)
    {
    this->ResetButton->Delete();
    this->ResetButton = NULL;
    }

  if (this->OpenButton)
    {
    this->OpenButton->Delete();
    this->OpenButton = NULL;
    }

  if (this->XLabel)
    {
    this->XLabel->Delete();
    this->XLabel = NULL;
    }
  if (this->XEntry)
    {
    this->XEntry->Delete();
    this->XEntry = NULL;
    }

  if (this->YLabel)
    {
    this->YLabel->Delete();
    this->YLabel = NULL;
    }
  if (this->YEntry)
    {
    this->YEntry->Delete();
    this->YEntry = NULL;
    }

  if (this->ZLabel)
    {
    this->ZLabel->Delete();
    this->ZLabel = NULL;
    }
  if (this->ZEntry)
    {
    this->ZEntry->Delete();
    this->ZEntry = NULL;
    }

  if (this->CloseButton)
    {
    this->CloseButton->Delete();
    this->CloseButton = NULL;
    }


  this->SetRenderView(NULL);

  if (this->Picker)
    {
    this->Picker->Delete();
    this->Picker = NULL;
    }

  if (this->CenterActor)
    {
    this->CenterActor->Delete();
    this->CenterActor = NULL;
    }

  if (this->CenterMapper)
    {
    this->CenterMapper->Delete();
    this->CenterMapper = NULL;
    }

  if (this->CenterSource)
    {
    this->CenterSource->Delete();
    this->CenterSource = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // create the main frame for this widget
  this->Script( "frame %s -bd 0", this->GetWidgetName());

  this->PickButton->Create(app,"button","-image KWPickCenterButton -bd 1");
  this->PickButton->SetCommand(this, "PickCallback");
  this->Script( "pack %s -side left -expand 0 -fill none -pady 2",
                this->PickButton->GetWidgetName());
  this->PickButton->SetBalloonHelpString(
    "Pick a new center of rotation by picking a point in the view.  The new point will be on the surface of a part.");
  
  this->ResetButton->Create(app,"button","-text {Reset} -bd 1");
  this->ResetButton->SetCommand(this, "ResetCallback");
  this->Script( "pack %s -side left -expand 0 -fill none -pady 2",
                this->ResetButton->GetWidgetName());
  // Since the reset button behaves differently for view and parts,
  // leave the help setup to the interactors.
  
  this->OpenButton->Create(app,"button","-text {>} -bd 1");
  this->OpenButton->SetCommand(this, "OpenCallback");
  this->OpenButton->SetBalloonHelpString("Expand the center of rotation user interface");
  this->Script( "pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
                this->OpenButton->GetWidgetName());
  
  // Point entry
  this->XLabel->Create(app,"label","-text {X}");
  this->XLabel->SetBalloonHelpString("Set the X-coordinate for the center of rotation");
  this->XEntry->Create(app,"-width 7");
  this->XEntry->SetBalloonHelpString("Set the X-coordinate for the center of rotation");
  this->Script( "bind %s <KeyPress-Return> {%s EntryCallback}",
                this->XEntry->GetWidgetName(), this->GetTclName());
  
  this->YLabel->Create(app,"label","-text {Y}");
  this->YLabel->SetBalloonHelpString("Set the Y-coordinate for the center of rotation");
  this->YEntry->Create(app,"-width 7");
  this->YEntry->SetBalloonHelpString("Set the Y-coordinate for the center of rotation");
  this->Script( "bind %s <KeyPress-Return> {%s EntryCallback}",
                this->YEntry->GetWidgetName(), this->GetTclName());
  
  this->ZLabel->Create(app,"label","-text {Z}");
  this->ZLabel->SetBalloonHelpString("Set the Z-coordinate for the center of rotation");
  this->ZEntry->Create(app,"-width 7");
  this->ZEntry->SetBalloonHelpString("Set the Z-coordinate for the center of rotation");
  this->Script( "bind %s <KeyPress-Return> {%s EntryCallback}",
                this->ZEntry->GetWidgetName(), this->GetTclName());
  
  this->CloseButton->Create(app,"button","-text {<} -bd 1");
  this->CloseButton->SetCommand(this, "CloseCallback");
  this->CloseButton->SetBalloonHelpString("Collapse the center of rotation user interface");
  
  this->Update();
}


//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::OpenCallback()
{
  this->Script("pack forget %s", this->OpenButton->GetWidgetName());

  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->XLabel->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->XEntry->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->YLabel->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->YEntry->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->ZLabel->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->ZEntry->GetWidgetName());

  this->Script("pack %s -side left -expand 0 -fill none -padx 2 -pady 2",
               this->CloseButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::CloseCallback()
{
  this->Script("pack forget %s", this->XLabel->GetWidgetName());
  this->Script("pack forget %s", this->XEntry->GetWidgetName());
  this->Script("pack forget %s", this->YLabel->GetWidgetName());
  this->Script("pack forget %s", this->YEntry->GetWidgetName());
  this->Script("pack forget %s", this->ZLabel->GetWidgetName());
  this->Script("pack forget %s", this->ZEntry->GetWidgetName());

  this->Script("pack forget %s", this->CloseButton->GetWidgetName());

  this->Script("pack %s -side left -expand 0 -fill none",
               this->OpenButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::PickCallback()
{
  // Just in case
  if (this->RenderView == NULL)
    {
    return;
    }

  // Send me the events.
  this->RenderView->SetInteractor(this);
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::ResetCallback()
{
  if (this->CameraInteractor)
    {
    double center[4];
    this->ComputeScreenCenter(center);
    this->CameraInteractor->SetCenter(center[0], 
                                      center[1], center[2]);
    this->Update();
    this->RenderView->EventuallyRender();
    }
  this->DefaultFlag = 1;
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::ShowCenterOn()
{
  this->Update();
  this->CenterActor->VisibilityOn();
  this->CenterSource->Modified();
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::ShowCenterOff()
{
  this->CenterActor->VisibilityOff();
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::CameraMovedNotify()
{
  if (this->CameraInteractor && this->DefaultFlag)
    {
    this->ResetCallback();
    }
}



//----------------------------------------------------------------------------
void 
vtkKWCenterOfRotation::SetCameraInteractor(vtkCameraInteractor *interactor)
{
  if (this->CameraInteractor == interactor)
    {
    return;
    }
  if (this->CameraInteractor)
    {
    this->CameraInteractor->UnRegister(this);
    }
  this->CameraInteractor = interactor;
  if (interactor)
    {
    double c[3];
    interactor->Register(this);
    interactor->GetCenter(c);
    this->CenterActor->SetPosition(c[0], c[1], c[3]);
    if (this->Application)
      {
      this->XEntry->SetValue(c[0], 5);
      this->YEntry->SetValue(c[1], 5);
      this->ZEntry->SetValue(c[2], 5);
      this->RenderView->EventuallyRender();
      }
    }
}


//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::ComputeScreenCenter(double center[3])
{
  int tmp;

  if (this->RenderView == NULL)
    {
    return;
    }

  tmp = this->CenterActor->GetVisibility();
  this->CenterActor->SetVisibility(0);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  vtkCamera *cam = this->RenderView->GetRenderer()->GetActiveCamera();
  double *range = cam->GetClippingRange();
  double mid = 0.5 * (range[0] + range[1]);
  double *pos = cam->GetPosition();
  double *norm = cam->GetDirectionOfProjection();
  center[0] = pos[0] + mid * norm[0];
  center[1] = pos[1] + mid * norm[1];
  center[2] = pos[2] + mid * norm[2];

  this->CenterActor->SetVisibility(tmp);
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::SetRenderView(vtkPVRenderView *view)
{
  vtkRenderer *ren;

  // Take care of adding or removing actor.
  if (this->RenderView != NULL)
    {
    ren = this->RenderView->GetRenderer();
    if (ren)
      {
      ren->RemoveActor(this->CenterActor);
      }
    }
  if (view != NULL)
    {
    ren = view->GetRenderer();
    if (ren)
      {
      ren->AddActor(this->CenterActor);
      this->ResetCenterActorSize();
      }
    }
  
  // Picker needs a link to the Tree Composite class.
  if (view)
    {
    this->Picker->SetComposite(view->GetComposite());
    }

  // Now just set the render view with reference counting
  this->vtkKWInteractor::SetRenderView(view);
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::ResetCenterActorSize()
{
  vtkActorCollection *actors;
  vtkActor *a;
  float bounds[6], *temp;
  int idx, firstFlag;
  vtkRenderer *ren;

  if (this->RenderView == NULL)
    {
    return;
    }
  ren = this->RenderView->GetRenderer();
  if (ren == NULL)
    {
    return;
    }
  
  firstFlag = 1;
  // loop through all visible actors
  actors = ren->GetActors();
  actors->InitTraversal();
  while ((a = actors->GetNextItem()))
    {
    if (a->GetVisibility() && a->GetPickable())
      {
      temp = a->GetBounds();
      for (idx = 0; idx < 3; ++idx)
        {
        if (firstFlag || temp[idx*2] < bounds[idx*2])
          {
          bounds[idx*2] = temp[idx*2];
          }
        if (firstFlag || temp[idx*2 + 1] > bounds[idx*2 + 1])
          {
          bounds[idx*2 + 1] = temp[idx*2 + 1];
          }
        }
        firstFlag = 0;
      }
    }

  // now use bounds to set the size fot the center cursor actor
  if ( ! firstFlag)
    {
    this->CenterActor->SetScale(0.25 * (bounds[1]-bounds[0]), 
                                0.25 * (bounds[3]-bounds[2]),
                                0.25 * (bounds[5]-bounds[4]));
    }
}

//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::Update()
{
  double center[3];
  
  if (this->CameraInteractor == NULL)
    {
    return;
    }

  if (this->CameraInteractor)
    {
    if (this->DefaultFlag)
      {
      this->ComputeScreenCenter(center);
      this->CameraInteractor->SetCenter(center);
      }
    else
      {
      this->CameraInteractor->GetCenter(center);
      }
    }

  this->CenterActor->SetPosition(center[0], center[1], center[2]);
  this->ResetCenterActorSize();

  if (this->Application)
    {
    this->XEntry->SetValue(center[0], 4);
    this->YEntry->SetValue(center[1], 4);
    this->ZEntry->SetValue(center[2], 4);
    } 
}



//----------------------------------------------------------------------------
// I put these here for bounding box pick which is not implemented yet.
void vtkKWCenterOfRotation::AButtonPress(int num, int x, int y)
{
}
//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::Button1Motion(int x, int y)
{
}

//----------------------------------------------------------------------------
// Right now it just picks a point, but I intend to pick parts
// with the right button (3) in the future.
void vtkKWCenterOfRotation::AButtonRelease(int num, int x, int y)
{
  float center[3];
  int *size;

  if (this->RenderView == NULL)
    {
    return;
    }
  vtkRenderer *ren = this->RenderView->GetRenderer();
  if (ren == NULL)
    {
    return;
    }

  size = ren->GetSize();
  // Move origin to lower left.
  y = size[1] - y;
  if (num == 1)
    {
    this->Picker->Pick(x, y, 0.0, ren);
    this->Picker->GetPickPosition(center);
    if (this->CameraInteractor)
      {
      this->CameraInteractor->SetCenter(center[0], 
                                  center[1], center[2]);
      }
    this->DefaultFlag = 0;

    this->Update();
    this->RenderView->EventuallyRender();
    this->RenderView->SetInteractor(this->ParentInteractor);
    }

}


//----------------------------------------------------------------------------
void vtkKWCenterOfRotation::EntryCallback()
{
  double center[3];

  center[0] = this->XEntry->GetValueAsFloat();
  center[1] = this->YEntry->GetValueAsFloat();
  center[2] = this->ZEntry->GetValueAsFloat();

  if (this->CameraInteractor)
    {
    this->CameraInteractor->SetCenter(center[0], 
                                      center[1], center[2]);
    }
  this->DefaultFlag = 0;
  this->Update();
  this->RenderView->EventuallyRender();
}
