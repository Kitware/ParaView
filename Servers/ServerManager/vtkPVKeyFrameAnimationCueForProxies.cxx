/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVKeyFrameAnimationCueForProxies.h"

#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkPVKeyFrameAnimationCueForProxies);
vtkCxxSetObjectMacro(vtkPVKeyFrameAnimationCueForProxies,
  AnimatedProxy, vtkSMProxy);
//----------------------------------------------------------------------------
vtkPVKeyFrameAnimationCueForProxies::vtkPVKeyFrameAnimationCueForProxies()
{
  this->AnimatedProxy = 0;
  this->AnimatedPropertyName = 0;
  this->AnimatedDomainName = 0;
}

//----------------------------------------------------------------------------
vtkPVKeyFrameAnimationCueForProxies::~vtkPVKeyFrameAnimationCueForProxies()
{
  this->SetAnimatedProxy(0);
  this->SetAnimatedPropertyName(0);
  this->SetAnimatedDomainName(0);
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::RemoveAnimatedProxy()
{
  this->SetAnimatedProxy(0);
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVKeyFrameAnimationCueForProxies::GetAnimatedProperty()
{
  if (!this->AnimatedPropertyName || !this->AnimatedProxy)
    {
    return NULL;
    }

  return this->AnimatedProxy->GetProperty(this->AnimatedPropertyName);
}

//----------------------------------------------------------------------------
vtkSMDomain* vtkPVKeyFrameAnimationCueForProxies::GetAnimatedDomain()
{
  vtkSMProperty* property = this->GetAnimatedProperty();
  if (!property)
    {
    return NULL;
    }
  vtkSMDomain* domain = NULL;
  vtkSMDomainIterator* iter = property->NewDomainIterator();
  iter->Begin();
  if (!iter->IsAtEnd())
    {
    domain = iter->GetDomain();
    }
  iter->Delete();
  return domain;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::BeginUpdateAnimationValues()
{
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::SetAnimationValue(
  int index, double value)
{

  vtkSMDomain *domain = this->GetAnimatedDomain();
  vtkSMProperty *property = this->GetAnimatedProperty();
  vtkSMProxy *proxy = this->GetAnimatedProxy();
  if (!proxy || !domain || !property)
    {
    vtkErrorMacro("Cue does not have domain or property set!");
    return;
    }
  domain->SetAnimationValue(property, index, value);
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::EndUpdateAnimationValues()
{
  // FIXME_COLLABORATION:
  // How can the keyframe tell the cue that the property needs to be
  // resized or not?
  //vtkSMVectorProperty * vp = vtkSMVectorProperty::SafeDownCast(property);
  //if(vp)
  //  {
  //  vp->SetNumberOfElements(max);
  //  }
  if (this->AnimatedProxy)
    {
    this->AnimatedProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
