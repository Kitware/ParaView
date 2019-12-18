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

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVSinusoidKeyFrame.h"

vtkStandardNewMacro(vtkPVCompositeKeyFrame);
//-----------------------------------------------------------------------------
vtkPVCompositeKeyFrame::vtkPVCompositeKeyFrame()
{
  this->Type = RAMP;
  this->BooleanKeyFrame = vtkPVBooleanKeyFrame::New();
  this->RampKeyFrame = vtkPVRampKeyFrame::New();
  this->ExponentialKeyFrame = vtkPVExponentialKeyFrame::New();
  this->SinusoidKeyFrame = vtkPVSinusoidKeyFrame::New();

  // Whenever any of the internal keyframes is modified, we fired a modified
  // event as well.
  this->BooleanKeyFrame->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVCompositeKeyFrame::Modified);
  this->RampKeyFrame->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVCompositeKeyFrame::Modified);
  this->ExponentialKeyFrame->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVCompositeKeyFrame::Modified);
  this->SinusoidKeyFrame->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVCompositeKeyFrame::Modified);
}

//-----------------------------------------------------------------------------
vtkPVCompositeKeyFrame::~vtkPVCompositeKeyFrame()
{
  this->BooleanKeyFrame->Delete();
  this->RampKeyFrame->Delete();
  this->ExponentialKeyFrame->Delete();
  this->SinusoidKeyFrame->Delete();
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
void vtkPVCompositeKeyFrame::UpdateValue(
  double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next)
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
      this->Superclass::UpdateValue(currenttime, cue, next);
  }
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Type: " << this->GetTypeAsString(this->Type) << endl;
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::RemoveAllKeyValues()
{
  this->BooleanKeyFrame->RemoveAllKeyValues();
  this->RampKeyFrame->RemoveAllKeyValues();
  this->ExponentialKeyFrame->RemoveAllKeyValues();
  this->SinusoidKeyFrame->RemoveAllKeyValues();
  this->Superclass::RemoveAllKeyValues();
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetKeyTime(double time)
{
  this->BooleanKeyFrame->SetKeyTime(time);
  this->RampKeyFrame->SetKeyTime(time);
  this->ExponentialKeyFrame->SetKeyTime(time);
  this->SinusoidKeyFrame->SetKeyTime(time);
  this->Superclass::SetKeyTime(time);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetKeyValue(unsigned int index, double val)
{
  this->BooleanKeyFrame->SetKeyValue(index, val);
  this->RampKeyFrame->SetKeyValue(index, val);
  this->ExponentialKeyFrame->SetKeyValue(index, val);
  this->SinusoidKeyFrame->SetKeyValue(index, val);
  this->Superclass::SetKeyValue(index, val);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetNumberOfKeyValues(unsigned int num)
{
  this->BooleanKeyFrame->SetNumberOfKeyValues(num);
  this->RampKeyFrame->SetNumberOfKeyValues(num);
  this->ExponentialKeyFrame->SetNumberOfKeyValues(num);
  this->SinusoidKeyFrame->SetNumberOfKeyValues(num);
  this->Superclass::SetNumberOfKeyValues(num);
}

// Passed on to the ExponentialKeyFrame.
//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetBase(double val)
{
  this->ExponentialKeyFrame->SetBase(val);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetStartPower(double val)
{
  this->ExponentialKeyFrame->SetStartPower(val);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetEndPower(double val)
{
  this->ExponentialKeyFrame->SetEndPower(val);
}

// Passed on to the SinusoidKeyFrame.
//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetPhase(double val)
{
  this->SinusoidKeyFrame->SetPhase(val);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetFrequency(double val)
{
  this->SinusoidKeyFrame->SetFrequency(val);
}

//-----------------------------------------------------------------------------
void vtkPVCompositeKeyFrame::SetOffset(double val)
{
  this->SinusoidKeyFrame->SetOffset(val);
}
