/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraIcon.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindowToImageFilter.h"

#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDoubleVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCameraIcon);
vtkCxxRevisionMacro(vtkPVCameraIcon, "1.22.2.3");

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
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVCameraIcon already created");
    return;
    }

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

  this->SetText("Empty");
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
  if ( this->RenderView && this->Camera )
    {
    vtkSMProxy* renModuleProxy = this->RenderView->GetRenderModuleProxy();
    vtkSMDoubleVectorProperty* dvp;
 
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraPosition"));
    if (dvp)
      {
      dvp->SetElements(this->Camera->GetPosition());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraPosition.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraFocalPoint"));
    if (dvp)
      {
      dvp->SetElements(this->Camera->GetFocalPoint());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPoint.");
      }
  
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraViewUp"));
    if (dvp)
      {
      dvp->SetElements(this->Camera->GetViewUp());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPoint.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraViewAngle"));
    if (dvp)
      {
      dvp->SetElement(0, this->Camera->GetViewAngle());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraViewAngle.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraClippingRange"));
    if (dvp)
      {
      dvp->SetElements(this->Camera->GetClippingRange());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraClippingRange.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraParallelScale"));
    if (dvp)
      {
      dvp->SetElement(0, this->Camera->GetParallelScale());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraParallelScale.");
      }
    renModuleProxy->UpdateVTKObjects(); 
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
    this->Camera = vtkCamera::New();
    
    vtkSMProxy* renModuleProxy = this->RenderView->GetRenderModuleProxy();
    renModuleProxy->UpdateInformation();
    vtkSMDoubleVectorProperty* dvp;

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraPositionInfo"));
    if (dvp)
      {
      this->Camera->SetPosition(dvp->GetElements());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraPositionInfo.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraFocalPointInfo"));
    if (dvp)
      {
      this->Camera->SetFocalPoint(dvp->GetElements());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPointInfo.");
      }
  
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraViewUpInfo"));
    if (dvp)
      {
      this->Camera->SetViewUp(dvp->GetElements());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraFocalPointInfo.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraViewAngleInfo"));
    if (dvp)
      {
      this->Camera->SetViewAngle(dvp->GetElement(0));
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraViewAngleInfo.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraClippingRangeInfo"));
    if (dvp)
      {
      this->Camera->SetClippingRange(dvp->GetElements());
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraClippingRangeInfo.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      renModuleProxy->GetProperty("CameraParallelScaleInfo"));
    if (dvp)
      {
      this->Camera->SetParallelScale(dvp->GetElement(0));
      }
    else
      {
      vtkErrorMacro("Failed to find property CameraParallelScaleInfo.");
      }

    vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
    w2i->SetInput(this->RenderView->GetRenderWindow());
    w2i->ShouldRerenderOff();
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
