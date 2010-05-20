/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExponentialKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExponentialKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"

#include <math.h>

vtkStandardNewMacro(vtkSMExponentialKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMExponentialKeyFrameProxy::vtkSMExponentialKeyFrameProxy()
{
  this->Base = 2.0;
  this->StartPower = -1.0;
  this->EndPower = -5.0;
}

//----------------------------------------------------------------------------
vtkSMExponentialKeyFrameProxy::~vtkSMExponentialKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMExponentialKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }

  if (this->Base == 1)
    {
    vtkErrorMacro("Exponential with base 1");
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

  double tcur = pow(this->Base, this->StartPower + currenttime * 
    (this->EndPower- this->StartPower)); 
  double tmin = pow(this->Base, this->StartPower);
  double tmax = pow(this->Base, this->EndPower);

  double t = (this->Base!= 1) ? (tcur - tmin) / (tmax - tmin): 0;

  if (animated_element != -1)
    {
    double vmax = next->GetKeyValue();
    double vmin = this->GetKeyValue();
    double value = vmin + t * (vmax - vmin);

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
      double value = vmin + t * (vmax - vmin);
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
void vtkSMExponentialKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Base: " << this->Base << endl;
  os << indent << "StartPower: " << this->StartPower << endl;
  os << indent << "EndPower: " << this->EndPower << endl;
}
