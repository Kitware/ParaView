/*=========================================================================

  Program:   ParaView
  Module:    vtkKWInteractor.cxx
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
#include "vtkPVRenderView.h"
#include "vtkKWToolbar.h"
#include "vtkKWInteractor.h"

int vtkKWInteractorCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWInteractor::vtkKWInteractor()
{
  this->CommandFunction = vtkKWInteractorCommand;
  this->RenderView = NULL;
  this->SelectedState = 0;
  this->ToolbarButton = NULL;
  this->Toolbar = NULL;
  
  this->TraceInitialized = 0;
  this->Tracing = 0;
}

//----------------------------------------------------------------------------
vtkKWInteractor::~vtkKWInteractor()
{
  this->SetRenderView(NULL);
  this->SetToolbarButton(NULL);
  if (this->Toolbar)
    {
    this->Toolbar->Delete();
    this->Toolbar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Create(vtkKWApplication *app, char *args)
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

}


//----------------------------------------------------------------------------
// Lets not "Register" the RenderView.
void vtkKWInteractor::SetRenderView(vtkPVRenderView *view)
{
  this->RenderView = view;
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Select()
{
  if (this->SelectedState)
    {
    return;
    }
  if (this->ToolbarButton)
    {
    this->ToolbarButton->SetState(1);
    }
  if (this->Toolbar)
    {
    this->Script("pack %s -side left -expand no -fill none",
                 this->Toolbar->GetWidgetName());
    }
  this->SelectedState = 1;
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Deselect()
{
  if ( ! this->SelectedState)
    {
    return;
    }
  if (this->ToolbarButton)
    {
    this->ToolbarButton->SetState(0);
    }
  if (this->Toolbar)
    {
    this->Script("pack forget %s", this->Toolbar->GetWidgetName());
    }
  this->SelectedState = 0;
}

//----------------------------------------------------------------------------
void vtkKWInteractor::SetCameraState(float p0, float p1, float p2,
                                     float fp0, float fp1, float fp2,
                                     float up0, float up1, float up2)
{
  vtkCamera *cam;

  if (this->RenderView == NULL)
    {
    return;
    }

  // This is to trace effects of loaded scripts.
  this->AddTraceEntry("$kw(%s) SetCameraState %.3f %.3f %.3f  %.3f %.3f %.3f  %.3f %.3f %.3f",
                      this->GetTclName(), p0, p1, p2, fp0, fp1, fp2, up0, up1, up2);

  cam = this->RenderView->GetRenderer()->GetActiveCamera();
  cam->SetPosition(p0, p1, p2);
  cam->SetFocalPoint(fp0, fp1, fp2);
  cam->SetViewUp(up0, up1, up2);

  this->RenderView->EventuallyRender();
}



