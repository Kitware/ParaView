/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTranslateCameraInteractor.cxx
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
#include "vtkKWTranslateCameraInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

int vtkKWTranslateCameraInteractorCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);


#define VTK_VIEW_PAN 1
#define VTK_VIEW_ZOOM 2

//----------------------------------------------------------------------------
vtkKWTranslateCameraInteractor::vtkKWTranslateCameraInteractor()
{
  this->CommandFunction = vtkKWTranslateCameraInteractorCommand;
  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this);

  this->Helper = vtkCameraInteractor::New();
  this->PanCursorName = NULL;
  this->ZoomCursorName = NULL;
  this->CursorState = 0;
}

//----------------------------------------------------------------------------
vtkKWTranslateCameraInteractor *vtkKWTranslateCameraInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWTranslateCameraInteractor");
  if(ret)
    {
    return (vtkKWTranslateCameraInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWTranslateCameraInteractor;
}


//----------------------------------------------------------------------------
vtkKWTranslateCameraInteractor::~vtkKWTranslateCameraInteractor()
{
  this->Label->Delete();
  this->Label = NULL;

  this->Helper->Delete();
  this->Helper = NULL;
  this->SetPanCursorName(NULL);
  this->SetZoomCursorName(NULL);
}


//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // create the main frame for this widget
  this->Script( "frame %s", this->GetWidgetName());
  
  this->Label->Create(app,"label","-text {TranslateCamera}");
  this->Script( "pack %s -side top -expand 0 -fill x",
                this->Label->GetWidgetName());

  this->InitializeCursors();  
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Select()
{
  if (this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Select();

  // Should change the cursor here -- done in MotionCallback
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Deselect()
{
  if ( ! this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Deselect();

  // Should change the cursor here
  this->Script("%s configure -cursor top_left_arrow",
               this->RenderView->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::AButtonPress(int num, int x, int y)
{
  if (this->RenderView == NULL)
    {
    return;
    }
  vtkRenderer *ren = this->RenderView->GetRenderer();
  if (ren == NULL)
    {
    return;
    }
  this->Helper->SetRenderer(ren);

  if (num == 1)
    {
    this->RenderView->SetRenderModeToInteractive();
    this->Helper->PanZoomStart(x, y);
    }
  if (num == 3)
    {
    this->Script("%s configure -cursor %s",
                 this->RenderView->GetWidgetName(), this->ZoomCursorName);
    this->CursorState = VTK_VIEW_ZOOM;
    this->RenderView->SetRenderModeToInteractive();
    this->Helper->ZoomStart(x, y);
    }

}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Button1Motion(int x, int y)
{
  this->Helper->PanZoomMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render(); 
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Button3Motion(int x, int y)
{
  this->Helper->ZoomMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render();
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::AButtonRelease(int num, int x, int y)
{
  if (this->RenderView == NULL)
    {
    return;
    }

  this->RenderView->EventuallyRender(); 
}


//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::MotionCallback(int x, int y)
{
  int *size = this->RenderView->GetRenderer()->GetSize();
  double pos;

  //this->Script("%s configure -cursor watch",
  //    this->RenderView->GetWidgetName());

  pos = (double)y / (double)(size[1]);

  char str[1000];
  sprintf(str, "%d, %d, %f", y, size[1], pos);
  this->RenderView->GetWindow()->SetStatusText(str);

  if (pos < 0.333 || pos > 0.667)
    {
    if (this->CursorState != VTK_VIEW_ZOOM)
      {
      this->CursorState = VTK_VIEW_ZOOM;
      this->Script("%s configure -cursor %s",
                   this->RenderView->GetWidgetName(),
                   this->ZoomCursorName);
      }
    }
  else
    {
    if (this->CursorState != VTK_VIEW_PAN)
      {
      this->CursorState = VTK_VIEW_PAN;
      this->Script("%s configure -cursor %s",
                   this->RenderView->GetWidgetName(),
                   this->PanCursorName);
      }
    }      
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::InitializeCursors()
{
#ifdef _WIN32
  this->SetZoomCursorName("size_ns");
#else
  //this->SetZoomCursorName("sb_h_double_arrow");
  this->SetZoomCursorName("sb_v_double_arrow");
#endif

  this->SetPanCursorName("fleur");
}


