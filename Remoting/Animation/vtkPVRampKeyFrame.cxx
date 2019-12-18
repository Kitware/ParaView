/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRampKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRampKeyFrame.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"

vtkStandardNewMacro(vtkPVRampKeyFrame);
//----------------------------------------------------------------------------
vtkPVRampKeyFrame::vtkPVRampKeyFrame()
{
}

//----------------------------------------------------------------------------
vtkPVRampKeyFrame::~vtkPVRampKeyFrame()
{
}

//----------------------------------------------------------------------------
// remember that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkPVRampKeyFrame::UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next)
{
  if (!next)
  {
    return;
  }

  cue->BeginUpdateAnimationValues();
  int animated_element = cue->GetAnimatedElement();
  if (animated_element != -1)
  {
    double vmax = next->GetKeyValue();
    double vmin = this->GetKeyValue();
    double value = vmin + currenttime * (vmax - vmin);
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
      double vmax = next->GetKeyValue(i);
      double vmin = this->GetKeyValue(i);
      double value = vmin + currenttime * (vmax - vmin);
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
void vtkPVRampKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
