/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCameraIcon.cxx
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
#include "vtkPVCameraIcon.h"

#include "vtkCamera.h"
#include "vtkImageResample.h"
#include "vtkKWIcon.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCameraIcon);

vtkCxxSetObjectMacro(vtkPVCameraIcon,RenderView,vtkPVRenderView);

//----------------------------------------------------------------------------
vtkPVCameraIcon::vtkPVCameraIcon()
{
  this->RenderView = 0;
  this->Camera = 0;
  this->Width = 60;
  this->Height = 50;
}

//----------------------------------------------------------------------------
vtkPVCameraIcon::~vtkPVCameraIcon()
{
  this->SetRenderView(0);
  if ( this->Camera )
    {
    this->Camera->Delete();
    this->Camera = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVCameraIcon::Create(vtkKWApplication *pvApp)
{
  this->Superclass::Create(pvApp, 0);
  this->Script("%s configure -relief raised", this->GetWidgetName());
  this->Script("%s configure -padx 0 -pady 0 -anchor center", 
               this->GetWidgetName());
  
  //this->Script("%s configure -width %d -height %d", 
  //             this->GetWidgetName(), this->Width, this->Height);
  this->Script("%s configure -anchor s", this->GetWidgetName());
  this->SetBind(this, "<Button-1>", "RestoreCamera");
  this->SetBind(this, "<Button-3>", "StoreCamera");
  this->SetBalloonHelpString(
    "Left mouse button retrieve camera; Right mouse button store camera");
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImageData(vtkKWIcon::ICON_EMPTY);
  this->SetImageData(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCameraIcon::RestoreCamera()
{
  if ( this->RenderView && this->Camera )
    {
    ostrstream str;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetParallelScale [[ "
        << this->GetTclName() << " GetCamera ] GetParallelScale ]" << endl;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetViewAngle [[ "
        << this->GetTclName() << " GetCamera ] GetViewAngle ]" << endl;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetClippingRange [[ "
        << this->GetTclName() << " GetCamera ] GetClippingRange ]" << endl;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetFocalPoint [[ "
        << this->GetTclName() << " GetCamera ] GetFocalPoint ]" << endl;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetPosition [[ "
        << this->GetTclName() << " GetCamera ] GetPosition ]" << endl;
    str << "eval [ " << this->RenderView->GetRendererTclName() 
        << " GetActiveCamera ] SetViewUp [[ "
        << this->GetTclName() << " GetCamera ] GetViewUp ]" << endl;
    str << this->RenderView->GetRendererTclName() << " ResetCameraClippingRange";
    str << ends;

    this->RenderView->GetPVApplication()->BroadcastScript(str.str());
    str.rdbuf()->freeze(0);
    this->RenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVCameraIcon::StoreCamera()
{
  if ( this->RenderView )
    {
    if ( this->Camera )
      {
      this->Camera->Delete();
      this->Camera = 0;
      }
    vtkCamera* cam = this->RenderView->GetRenderer()->GetActiveCamera();
    this->Camera = cam->NewInstance();
    this->Camera->SetParallelScale(cam->GetParallelScale());
    this->Camera->SetViewAngle(cam->GetViewAngle());
    this->Camera->SetClippingRange(cam->GetClippingRange());
    this->Camera->SetFocalPoint(cam->GetFocalPoint());
    this->Camera->SetPosition(cam->GetPosition());
    this->Camera->SetViewUp(cam->GetViewUp());

    vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
    w2i->SetInput(this->RenderView->GetRenderWindow());
    w2i->Update();

    int* dim = w2i->GetOutput()->GetDimensions();
    float width = dim[0];
    float height = dim[1];


    vtkImageResample *resample = vtkImageResample::New();    
    resample->SetAxisMagnificationFactor(0, 
                                         static_cast<float>(this->Width)/width);
    resample->SetAxisMagnificationFactor(1, 
                                         static_cast<float>(this->Height)/height);
    resample->SetInput(w2i->GetOutput());
    resample->Update();

    vtkKWIcon* icon = vtkKWIcon::New();
    icon->SetImageData(resample->GetOutput());
    this->SetImageData(icon);
    icon->Delete();
    
    resample->Delete();
    w2i->Delete();
    // Fix label
    char buff[100];
    sprintf(buff, "%p", this->Camera);
    this->SetLabel(buff);
    }
}

//-------------------------------------------------------------------------
void vtkPVCameraIcon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
