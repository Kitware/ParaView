/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBooleanKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBooleanKeyFrame.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"

vtkStandardNewMacro(vtkPVBooleanKeyFrame);

//----------------------------------------------------------------------------
vtkPVBooleanKeyFrame::vtkPVBooleanKeyFrame() = default;

//----------------------------------------------------------------------------
vtkPVBooleanKeyFrame::~vtkPVBooleanKeyFrame() = default;

//----------------------------------------------------------------------------
// remember that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkPVBooleanKeyFrame::UpdateValue(double, vtkPVAnimationCue* cue, vtkPVKeyFrame*)
{
  cue->BeginUpdateAnimationValues();
  int animated_element = cue->GetAnimatedElement();
  if (animated_element != -1)
  {
    cue->SetAnimationValue(animated_element, this->GetKeyValue());
  }
  else
  {
    unsigned int max = this->GetNumberOfKeyValues();
    for (unsigned int i = 0; i < max; i++)
    {
      cue->SetAnimationValue(i, this->GetKeyValue(i));
    }
  }
  cue->EndUpdateAnimationValues();
}

//----------------------------------------------------------------------------
void vtkPVBooleanKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
