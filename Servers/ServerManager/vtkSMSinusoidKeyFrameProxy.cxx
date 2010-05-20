/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSinusoidKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSinusoidKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"

#include <math.h>

vtkStandardNewMacro(vtkSMSinusoidKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMSinusoidKeyFrameProxy::vtkSMSinusoidKeyFrameProxy()
{
  this->Phase = 0;
  this->Frequency = 1;
  this->Offset = 0;
}

//----------------------------------------------------------------------------
vtkSMSinusoidKeyFrameProxy::~vtkSMSinusoidKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMSinusoidKeyFrameProxy::UpdateValue(double currenttime,
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

  // ( start + (end-start)*sin( 2*pi* (freq*t + phase/360) ) )
  double t = sin ( 8.0 * atan(static_cast<double>(1.0)) *  
    (this->Frequency* currenttime + this->Phase/360.0));

  if (animated_element != -1)
    {
    double amplitude = this->GetKeyValue();
    double value = this->Offset + amplitude*t;
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
      double amplitude = this->GetKeyValue(i);
      double value = this->Offset + amplitude*t;
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
void vtkSMSinusoidKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Frequency: " << this->Frequency << endl;
  os << indent << "Phase: " << this->Phase << endl;
  os << indent << "Offset: " << this->Offset << endl;
}
