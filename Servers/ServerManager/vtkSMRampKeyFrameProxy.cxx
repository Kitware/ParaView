/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRampKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRampKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMSourceProxy.h"
vtkStandardNewMacro(vtkSMRampKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMRampKeyFrameProxy::vtkSMRampKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
vtkSMRampKeyFrameProxy::~vtkSMRampKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMRampKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }
  vtkSMDomain *domain = cueProxy->GetAnimatedDomain();
  vtkSMProperty *property = cueProxy->GetAnimatedProperty();
  vtkSMProxy *proxy = cueProxy->GetAnimatedProxy();
  int animated_element = cueProxy->GetAnimatedElement();

  if (!proxy || !domain || !property)
    {
    vtkErrorMacro("Cue does not have domain or property set!");
    return;
    }

  if (animated_element != -1)
    {
    double vmax = next->GetKeyValue();
    double vmin = this->GetKeyValue();
    double value = vmin + currenttime * (vmax - vmin);
    domain->SetAnimationValue(property, animated_element, value);
    }
  else
    {
    unsigned int start_novalues = this->GetNumberOfKeyValues();
    unsigned int end_novalues = next->GetNumberOfKeyValues();
    unsigned int min = (start_novalues < end_novalues)? start_novalues :
      end_novalues;
    unsigned int i;
    // interpolate comman indices.
    for (i=0; i < min; i++)
      {
      double vmax = next->GetKeyValue(i);
      double vmin = this->GetKeyValue(i);
      double value = vmin + currenttime * (vmax - vmin);
      domain->SetAnimationValue(property, i, value);
      }
    // add any additional indices in start key frame.
    for (i = min; i < start_novalues; i++)
      {
      domain->SetAnimationValue(property, i, this->GetKeyValue(i));
      }
    vtkSMVectorProperty * vp = vtkSMVectorProperty::SafeDownCast(property);
    if(vp)
      {
      vp->SetNumberOfElements(start_novalues);
      }
    }
  proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMRampKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
