/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeKeyFrame.h"

#include "vtkObjectFactory.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVSinusoidKeyFrame.h"

vtkStandardNewMacro(vtkPVCompositeKeyFrame);
//-----------------------------------------------------------------------------
vtkPVCompositeKeyFrame::vtkPVCompositeKeyFrame()
{
  this->Type = RAMP;
  this->BooleanKeyFrame = vtkPVBooleanKeyFrame::New();
  this->RampKeyFrame =  vtkPVRampKeyFrame::New();
  this->ExponentialKeyFrame = vtkPVExponentialKeyFrame::New();
  this->SinusoidKeyFrame = vtkPVSinusoidKeyFrame::New();
//  this->TimeLink = vtkSMPropertyLink::New();
//  this->ValueLink = vtkSMPropertyLink::New();
}

//-----------------------------------------------------------------------------
vtkPVCompositeKeyFrame::~vtkPVCompositeKeyFrame()
{
  this->BooleanKeyFrame->Delete();
  this->RampKeyFrame->Delete();
  this->ExponentialKeyFrame->Delete();
  this->SinusoidKeyFrame->Delete();
//  this->TimeLink->Delete();
//  this->ValueLink->Delete();
}

//-----------------------------------------------------------------------------
const char* vtkPVCompositeKeyFrame::GetTypeAsString(int type)
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
    }

  return "Unknown";
}

//-----------------------------------------------------------------------------
int vtkPVCompositeKeyFrame::GetTypeFromString(const char* type)
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
  return NONE;
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::CreateVTKObjects()
{
  // FIXME ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  if (this->ObjectsCreated)
//    {
//    return;
//    }

//  this->Superclass::CreateVTKObjects();

//  if (!this->ObjectsCreated)
//    {
//    return;
//    }

//  vtkCommand* observer = vtkMakeMemberFunctionCommand(*this,
//    &vtkSMCompositeKeyFrameProxy::InvokeModified);

//  // Link properties between the subproxies.
//  for (int cc= NONE+1; cc <= SINUSOID; cc++)
//    {
//    vtkSMProxy* proxy = this->GetSubProxy(this->GetTypeAsString(cc));
//    if (!proxy)
//      {
//      vtkWarningMacro("Missing subproxy with name " << this->GetTypeAsString(cc));
//      continue;
//      }
//    proxy->AddObserver(vtkCommand::ModifiedEvent, observer);
//    this->TimeLink->AddLinkedProperty(proxy->GetProperty("KeyTime"),
//      vtkSMLink::OUTPUT);
//    this->ValueLink->AddLinkedProperty(proxy->GetProperty("KeyValues"),
//      vtkSMLink::OUTPUT);
//    }
//  observer->Delete();

//  this->TimeLink->AddLinkedProperty(this->GetProperty("KeyTime"),
//    vtkSMLink::INPUT);
//  this->ValueLink->AddLinkedProperty(this->GetProperty("KeyValues"),
//    vtkSMLink::INPUT);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::UpdateValue( double currenttime,
                                          vtkPVAnimationCue* cue,
                                          vtkPVKeyFrame* next)
{
  switch (this->Type)
    {
    case BOOLEAN:
      this->BooleanKeyFrame->UpdateValue(currenttime, cue, next);
      break;

    case RAMP:
      this->RampKeyFrame->UpdateValue(currenttime, cue, next);
      break;

    case EXPONENTIAL:
      this->ExponentialKeyFrame->UpdateValue(currenttime, cue, next);
      break;

    case SINUSOID:
      this->SinusoidKeyFrame->UpdateValue(currenttime, cue, next);
      break;

    default:
      this->Superclass::UpdateValue(currenttime, cueProxy, next);
    }
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Type: " << this->GetTypeAsString(this->Type) << endl;
}
