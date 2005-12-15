/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkObjectFactory.h"

#include "vtkCamera.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"

vtkCxxRevisionMacro(vtkSMCameraKeyFrameProxy, "1.3");
vtkStandardNewMacro(vtkSMCameraKeyFrameProxy);
//----------------------------------------------------------------------------
vtkSMCameraKeyFrameProxy::vtkSMCameraKeyFrameProxy()
{
  this->Camera = vtkCamera::New();
  
}

//----------------------------------------------------------------------------
vtkSMCameraKeyFrameProxy::~vtkSMCameraKeyFrameProxy()
{
  this->Camera->Delete();
}

// MACROS to simplify our life.
#define PropertyToCamera(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid "Info"));\
  if (dvp)\
    {\
    camera->Set##propertyid (dvp->GetElements());\
    }\
  else  \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid "Info");\
    }\
}

#define PropertyToCameraSingleElement(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid "Info"));\
  if (dvp)\
    {\
    camera->Set##propertyid (dvp->GetElement(0));\
    }\
  else  \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid "Info");\
    }\
}

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
//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetKeyValue(vtkSMProxy* cameraProxy)
{
  cameraProxy->UpdatePropertyInformation();

  PropertyToCamera(cameraProxy, this->Camera, Position);
  PropertyToCamera(cameraProxy, this->Camera, FocalPoint);
  PropertyToCamera(cameraProxy, this->Camera, ViewUp);
  PropertyToCamera(cameraProxy, this->Camera, ClippingRange);
  PropertyToCameraSingleElement(cameraProxy, this->Camera, ViewAngle);
  PropertyToCameraSingleElement(cameraProxy, this->Camera, ParallelScale);
  
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetPosition(double x, double y, double z)
{
  this->Camera->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetFocalPoint(double x, double y, double z)
{
  this->Camera->SetFocalPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetViewUp(double x, double y, double z)
{
  this->Camera->SetViewUp(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetViewAngle(double angle)
{
  this->Camera->SetViewAngle(angle);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);

  double *val = this->Camera->GetPosition();
  const char* batchName = this->GetSelfIDAsString();
  *file << "  [$pvTemp" << batchName << " GetProperty Position]"
        << " SetElements3 " << val[0] << " " << val[1] << " " << val[2] << endl;

  val = this->Camera->GetFocalPoint();
  *file << "  [$pvTemp" << batchName << " GetProperty FocalPoint]"
        << " SetElements3 " << val[0] << " " << val[1] << " " << val[2] << endl;
 
  val = this->Camera->GetViewUp();
  *file << "  [$pvTemp" << batchName << " GetProperty ViewUp]"
        << " SetElements3 " << val[0] << " " << val[1] << " " << val[2] << endl;

  double angle = this->Camera->GetViewAngle();
  *file << "  [$pvTemp" << batchName << " GetProperty ViewAngle]"
        << " SetElements1 " << angle << endl;

  *file << "  $pvTemp" << batchName << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
