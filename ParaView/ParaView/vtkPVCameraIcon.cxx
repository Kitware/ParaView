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
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkImageResample.h"
#include "vtkKWIcon.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPVRenderModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCameraIcon);
vtkCxxRevisionMacro(vtkPVCameraIcon, "1.10.2.1");

vtkCxxSetObjectMacro(vtkPVCameraIcon,RenderView,vtkPVRenderView);

//----------------------------------------------------------------------------
vtkPVCameraIcon::vtkPVCameraIcon()
{
  this->RenderView = 0;
  this->Camera = 0;
  // Let's make it square so that they can be used as icons later on...
  this->Width = this->Height = 48; 
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
void vtkPVCameraIcon::Create(vtkKWApplication *pvApp, const char *args)
{
  this->Superclass::Create(pvApp, args);

  this->SetBind(this, "<Button-1>", "RestoreCamera");
  this->SetBind(this, "<Button-3>", "StoreCamera");
  this->SetBalloonHelpString(
    "Click left mouse button to retrieve the camera position, "
    "right mouse button to store a camera position.");

  // Get the size of the label, and try to adjust the padding so that
  // its width/height match the expect icon size (cannot be done
  // through the -width and -height Tk option since the unit is a char)
  // The should work up to 1 pixel accuracy (since padding is all around,
  // the total added pad will be an even nb of pixels).

  this->SetLabel("Empty");
  this->Script("%s configure -relief raised -anchor center", 
               this->GetWidgetName());

  int rw, rh, padx, pady, bd;
  this->Script("concat [winfo reqwidth %s] [winfo reqheight %s] "
               "[%s cget -padx] [%s cget -pady] [%s cget -bd]",
               this->GetWidgetName(), this->GetWidgetName(), 
               this->GetWidgetName(), this->GetWidgetName(), 
               this->GetWidgetName());

  sscanf(this->GetApplication()->GetMainInterp()->result, 
         "%d %d %d %d %d", 
         &rw, &rh, &padx, &pady, &bd);
  
  this->Script("%s configure -padx %d -pady %d", 
               this->GetWidgetName(), 
               padx + (int)ceil((double)(this->Width  - rw) / 2.0) + bd, 
               pady + (int)ceil((double)(this->Height - rh) / 2.0) + bd);
}

//----------------------------------------------------------------------------
void vtkPVCameraIcon::RestoreCamera()
{
#if 0
  const char* renTclName;

  if ( this->RenderView && this->Camera )
    {
    renTclName = this->RenderView->GetPVApplication()->GetRenderModule()
      ->GetRendererTclName();
    vtkClientServerID rendererID = this->RenderView->GetPVApplication()->GetRenderModule()
      ->GetRendererID();
    vtk renderer = this->RenderView->GetPVApplication()->GetRenderModule()
      ->GetRendererID();
    
    ostrstream str;
    str << "eval [" << renTclName 
        << " GetActiveCamera ] SetParallelScale [[ "
        << this->GetTclName() << " GetCamera ] GetParallelScale ]" << endl;
    str << "eval [ " << renTclName 
        << " GetActiveCamera ] SetViewAngle [[ "
        << this->GetTclName() << " GetCamera ] GetViewAngle ]" << endl;
    str << "eval [ " << renTclName 
        << " GetActiveCamera ] SetClippingRange [[ "
        << this->GetTclName() << " GetCamera ] GetClippingRange ]" << endl;
    str << "eval [ " << renTclName 
        << " GetActiveCamera ] SetFocalPoint [[ "
        << this->GetTclName() << " GetCamera ] GetFocalPoint ]" << endl;
    str << "eval [ " << renTclName 
        << " GetActiveCamera ] SetPosition [[ "
        << this->GetTclName() << " GetCamera ] GetPosition ]" << endl;
    str << "eval [ " << renTclName 
        << " GetActiveCamera ] SetViewUp [[ "
        << this->GetTclName() << " GetCamera ] GetViewUp ]" << endl;
    str << renTclName << " ResetCameraClippingRange";
    str << ends;

    this->RenderView->GetPVApplication()->BroadcastScript(str.str());
    str.rdbuf()->freeze(0);
    this->RenderView->EventuallyRender();
    }
#endif
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
    resample->SetAxisMagnificationFactor(
      0, static_cast<float>(this->Width)/width);
    resample->SetAxisMagnificationFactor(
      1, static_cast<float>(this->Height)/height);
    resample->SetInput(w2i->GetOutput());
    resample->Update();

    vtkKWIcon* icon = vtkKWIcon::New();
    icon->SetImage(resample->GetOutput());
    this->SetImageOption(icon);
    icon->Delete();    
    resample->Delete();
    w2i->Delete();

    this->Script("%s configure -padx 0 -pady 0 -anchor center", 
                 this->GetWidgetName());
    }
}

//-------------------------------------------------------------------------
void vtkPVCameraIcon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
