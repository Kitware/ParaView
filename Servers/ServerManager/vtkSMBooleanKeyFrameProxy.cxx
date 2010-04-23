/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBooleanKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"
vtkStandardNewMacro(vtkSMBooleanKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMBooleanKeyFrameProxy::vtkSMBooleanKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBooleanKeyFrameProxy::~vtkSMBooleanKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMBooleanKeyFrameProxy::UpdateValue(double ,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* )
{
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
    domain->SetAnimationValue(property, animated_element, this->GetKeyValue());
    }
  else
    {
    unsigned int max = this->GetNumberOfKeyValues();
    for (unsigned int i=0; i < max; i++)
      {
      domain->SetAnimationValue(property, i, this->GetKeyValue(i));
      }
    vtkSMVectorProperty * vp = vtkSMVectorProperty::SafeDownCast(property);
    if(vp)
      {
      vp->SetNumberOfElements(max);
      }
    }
  proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMBooleanKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
