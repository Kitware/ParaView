/*=========================================================================

  Program:   ParaView
  Module:    vtkKWTranslateCameraInteractor.cxx
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
#include "vtkKWApplication.h"
#include "vtkPVApplication.h"
#include "vtkKWTranslateCameraInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkKWWindow.h"
#include "vtkPVWindow.h"
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
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (this->Tracing)
    {
    this->AddTraceEntry("$kw(%s) AButtonPress %d %d %d",
                         this->GetTclName(), num, x, y);
    }

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
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (this->Tracing)
    {
    this->AddTraceEntry("$kw(%s) Button1Motion %d %d",
                         this->GetTclName(), x, y);
    }

  this->Helper->PanZoomMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render(); 
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::Button3Motion(int x, int y)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (this->Tracing)
    {
    this->AddTraceEntry("$kw(%s) Button3Motion %d %d",
                         this->GetTclName(), x, y);
    }

  this->Helper->ZoomMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render();
}

//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::AButtonRelease(int num, int x, int y)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (this->Tracing)
    {
    this->AddTraceEntry("$kw(%s) AButtonRelease %d %d %d",
                         this->GetTclName(), num, x, y);
    }

  if (this->RenderView == NULL)
    {
    return;
    }

  this->RenderView->EventuallyRender(); 
}


//----------------------------------------------------------------------------
void vtkKWTranslateCameraInteractor::MotionCallback(int x, int y)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (this->Tracing)
    {
    this->AddTraceEntry("$kw(%s) MotionCallback %d %d",
                         this->GetTclName(), x, y);
    }

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

int vtkKWTranslateCameraInteractor::InitializeTrace()
{
  vtkKWWindow *pvWindow = this->GetWindow();
  
  if (this->TraceInitialized)
    {
    return 1;
    }

  if (this->Application && pvWindow)
    {
    this->TraceInitialized = 1;
    this->AddTraceEntry("set kw(%s) [$kw(%s) GetTranslateCameraInteractor]",
                         this->GetTclName(), pvWindow->GetTclName());
    return 1;
    }
  return 0;
}
