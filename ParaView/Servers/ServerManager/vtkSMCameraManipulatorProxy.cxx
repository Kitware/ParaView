/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraManipulatorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraManipulatorProxy.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMCameraManipulatorProxy);
vtkCxxRevisionMacro(vtkSMCameraManipulatorProxy, "1.3");
//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::vtkSMCameraManipulatorProxy()
{
  this->CameraInterpolator = vtkCameraInterpolator::New();
}

//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::~vtkSMCameraManipulatorProxy()
{
  this->CameraInterpolator->Delete();
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Initialize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Initialize(cue);
  int nos = this->GetNumberOfKeyFrames();
  this->CameraInterpolator->Initialize();
  if (nos < 2)
    {
    vtkErrorMacro("Too few keyframes to animate.");
    return;
    }
  
  for(int i=0; i < nos; i++)
    {
    vtkSMCameraKeyFrameProxy* kf = vtkSMCameraKeyFrameProxy::SafeDownCast(
      this->GetKeyFrameAtIndex(i));
    if (!kf)
      {
      vtkErrorMacro("All keyframes in a vtkSMCameraKeyFrameProxy must be "
        "vtkSMCameraKeyFrameProxy");
      continue;
      }
    this->CameraInterpolator->AddCamera(kf->GetKeyTime(), kf->GetCamera());
    }
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Finalize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Finalize(cue);
}

//------------------------------------------------------------------------------
#define CameraToProperty(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid));\
  if (dvp) \
    {\
    dvp->SetElements(camera->Get##propertyid());\
    }\
  else \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid );\
    }\
}

#define CameraToPropertySingleElement(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid));\
  if (dvp) \
    {\
    dvp->SetElement(0,camera->Get##propertyid());\
    }\
  else \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid );\
    }\
}
//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::UpdateValue(double currenttime,
  vtkSMAnimationCueProxy* cue)
{
  vtkSMProxy* cameraProxy = cue->GetAnimatedProxy();
  vtkCamera* interpolatedCamera = vtkCamera::New();
  this->CameraInterpolator->InterpolateCamera(currenttime, interpolatedCamera);
  CameraToProperty(cameraProxy, interpolatedCamera, Position); 
  CameraToProperty(cameraProxy, interpolatedCamera, FocalPoint); 
  CameraToProperty(cameraProxy, interpolatedCamera, ViewUp); 
  CameraToProperty(cameraProxy, interpolatedCamera, ClippingRange); 
  CameraToPropertySingleElement(cameraProxy, interpolatedCamera, ViewAngle);
  CameraToPropertySingleElement(cameraProxy, interpolatedCamera, ParallelScale);
  
  cameraProxy->UpdateVTKObjects(); 
  interpolatedCamera->Delete();
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
