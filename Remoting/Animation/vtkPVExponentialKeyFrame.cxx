/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExponentialKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExponentialKeyFrame.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"

#include <math.h>

vtkStandardNewMacro(vtkPVExponentialKeyFrame);
//----------------------------------------------------------------------------
vtkPVExponentialKeyFrame::vtkPVExponentialKeyFrame()
{
  this->Base = 2.0;
  this->StartPower = -1.0;
  this->EndPower = -5.0;
}

//----------------------------------------------------------------------------
vtkPVExponentialKeyFrame::~vtkPVExponentialKeyFrame() = default;

//----------------------------------------------------------------------------
// remember that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkPVExponentialKeyFrame::UpdateValue(
  double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next)
{
  if (!next)
  {
    return;
  }

  if (this->Base == 1)
  {
    vtkErrorMacro("Exponential with base 1");
  }

  // Do some computation
  int animated_element = cue->GetAnimatedElement();
  double tcur =
    pow(this->Base, this->StartPower + currenttime * (this->EndPower - this->StartPower));
  double tmin = pow(this->Base, this->StartPower);
  double tmax = pow(this->Base, this->EndPower);
  double t = (this->Base != 1) ? (tcur - tmin) / (tmax - tmin) : 0;

  // Apply changes
  cue->BeginUpdateAnimationValues();
  if (animated_element != -1)
  {
    double vmax = next->GetKeyValue();
    double vmin = this->GetKeyValue();
    double value = vmin + t * (vmax - vmin);
    cue->SetAnimationValue(animated_element, value);
  }
  else
  {
    unsigned int start_novalues = this->GetNumberOfKeyValues();
    unsigned int end_novalues = next->GetNumberOfKeyValues();
    unsigned int min = (start_novalues < end_novalues) ? start_novalues : end_novalues;
    unsigned int i;
    // interpolate common indices.
    for (i = 0; i < min; i++)
    {
      double vmax = next->GetKeyValue(i);
      double vmin = this->GetKeyValue(i);
      double value = vmin + t * (vmax - vmin);
      cue->SetAnimationValue(i, value);
    }
    // add any additional indices in start key frame.
    for (i = min; i < start_novalues; i++)
    {
      cue->SetAnimationValue(i, this->GetKeyValue(i));
    }
  }
  cue->EndUpdateAnimationValues();
}

//----------------------------------------------------------------------------
void vtkPVExponentialKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Base: " << this->Base << endl;
  os << indent << "StartPower: " << this->StartPower << endl;
  os << indent << "EndPower: " << this->EndPower << endl;
}
