/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeKeyFrameProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyLink.h"

vtkStandardNewMacro(vtkSMCompositeKeyFrameProxy);
vtkCxxRevisionMacro(vtkSMCompositeKeyFrameProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMCompositeKeyFrameProxy::vtkSMCompositeKeyFrameProxy()
{
  this->Type = RAMP;
  this->TimeLink = vtkSMPropertyLink::New();
  this->ValueLink = vtkSMPropertyLink::New();
}

//-----------------------------------------------------------------------------
vtkSMCompositeKeyFrameProxy::~vtkSMCompositeKeyFrameProxy()
{
  this->TimeLink->Delete();
  this->ValueLink->Delete();
}

//-----------------------------------------------------------------------------
const char* vtkSMCompositeKeyFrameProxy::GetTypeAsString(int type)
{
  switch (type)
    {
  case NONE:
    return "None";

  case BOOLEAN:
    return "Boolean";

  case RAMP:
    return "Ramp";

  case EXPONENTIAL:
    return "Exponential";

  case SINUSOID:
    return "Sinusoid";

  case CAMERA:
    return "Camera";
    }

  return "Unknown";
}

//-----------------------------------------------------------------------------
int vtkSMCompositeKeyFrameProxy::GetTypeFromString(const char* type)
{
  if (!type)
    {
    return NONE;
    }

  if (strcmp(type, "Boolean") == 0)
    {
    return BOOLEAN;
    }
  else if (strcmp(type, "Ramp") == 0)
    {
    return RAMP;
    }
  else if (strcmp(type, "Exponential") == 0)
    {
    return EXPONENTIAL;
    }
  else if (strcmp(type, "Sinusoid") == 0)
    {
    return SINUSOID;
    }
  else if (strcmp(type, "Camera") == 0)
    {
    return CAMERA;
    }
  return NONE; 
}

//-----------------------------------------------------------------------------
void vtkSMCompositeKeyFrameProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);

  if (!this->ObjectsCreated)
    {
    return;
    }

  this->TimeLink->AddLinkedProperty(this->GetProperty("KeyTime"),
    vtkSMLink::INPUT);
  this->ValueLink->AddLinkedProperty(this->GetProperty("KeyValues"),
    vtkSMLink::INPUT);

  // Link properties between the subproxies.
  for (int cc= NONE+1; cc <= CAMERA; cc++)
    {
    vtkSMProxy* proxy = this->GetSubProxy(this->GetTypeAsString(cc));
    if (!proxy)
      {
      vtkWarningMacro("Missing subproxy with name " << this->GetTypeAsString(cc));
      continue;
      }
    this->TimeLink->AddLinkedProperty(proxy->GetProperty("KeyTime"),
      vtkSMLink::OUTPUT);
    this->ValueLink->AddLinkedProperty(proxy->GetProperty("KeyValues"),
      vtkSMLink::OUTPUT);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  switch (this->Type)
    {
  case BOOLEAN:
  case RAMP:
  case EXPONENTIAL:
  case SINUSOID:
  case CAMERA:
      {
      vtkSMProxy* proxy = this->GetSubProxy(this->GetTypeAsString(this->Type));
      if (!proxy)
        {
        vtkErrorMacro("Invalid proxy type: " << this->Type);
        return;
        }
      vtkSMKeyFrameProxy::SafeDownCast(proxy)->UpdateValue(
        currenttime, cueProxy, next);
      }
    break;

  default:
    this->Superclass::UpdateValue(currenttime, cueProxy, next);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Type: " << this->GetTypeAsString(this->Type) << endl;
}
