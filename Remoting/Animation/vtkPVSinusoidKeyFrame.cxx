/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSinusoidKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSinusoidKeyFrame.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"

//#include "vtkSMDomain.h"
//#include "vtkSMProxy.h"
//#include "vtkSMVectorProperty.h"

#include <math.h>

vtkStandardNewMacro(vtkPVSinusoidKeyFrame);
//----------------------------------------------------------------------------
vtkPVSinusoidKeyFrame::vtkPVSinusoidKeyFrame()
{
  this->Phase = 0;
  this->Frequency = 1;
  this->Offset = 0;
}

//----------------------------------------------------------------------------
vtkPVSinusoidKeyFrame::~vtkPVSinusoidKeyFrame() = default;

//----------------------------------------------------------------------------
// remember that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkPVSinusoidKeyFrame::UpdateValue(
  double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next)
{
  if (!next)
  {
    return;
  }

  // Some computations: start + (end-start)*sin( 2*pi* (freq*t + phase/360) )
  double t = sin(
    8.0 * atan(static_cast<double>(1.0)) * (this->Frequency * currenttime + this->Phase / 360.0));

  // Apply changes
  cue->BeginUpdateAnimationValues();
  int animated_element = cue->GetAnimatedElement();
  if (animated_element != -1)
  {
    double amplitude = this->GetKeyValue();
    double value = this->Offset + amplitude * t;
    cue->SetAnimationValue(animated_element, value);
  }
  else
  {
    unsigned int i;
    unsigned int start_novalues = this->GetNumberOfKeyValues();
    unsigned int end_novalues = next->GetNumberOfKeyValues();
    unsigned int min = (start_novalues < end_novalues) ? start_novalues : end_novalues;
    // interpolate common indices.
    for (i = 0; i < min; i++)
    {
      double amplitude = this->GetKeyValue(i);
      double value = this->Offset + amplitude * t;
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
void vtkPVSinusoidKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Frequency: " << this->Frequency << endl;
  os << indent << "Phase: " << this->Phase << endl;
  os << indent << "Offset: " << this->Offset << endl;
}
