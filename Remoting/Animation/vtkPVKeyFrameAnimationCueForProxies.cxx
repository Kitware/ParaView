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
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPVKeyFrameAnimationCueForProxies);
vtkCxxSetObjectMacro(vtkPVKeyFrameAnimationCueForProxies, AnimatedProxy, vtkSMProxy);
//----------------------------------------------------------------------------
vtkPVKeyFrameAnimationCueForProxies::vtkPVKeyFrameAnimationCueForProxies()
  : AnimatedProxy(nullptr)
  , AnimatedPropertyName(nullptr)
  , AnimatedDomainName(nullptr)
  , ValueIndexMax(-1)
{
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
    return nullptr;
  }

  return this->AnimatedProxy->GetProperty(this->AnimatedPropertyName);
}

//----------------------------------------------------------------------------
vtkSMDomain* vtkPVKeyFrameAnimationCueForProxies::GetAnimatedDomain()
{
  vtkSMProperty* property = this->GetAnimatedProperty();
  if (!property)
  {
    return nullptr;
  }

  auto iter = vtkSmartPointer<vtkSMDomainIterator>::Take(property->NewDomainIterator());
  iter->Begin();
  if (!iter->IsAtEnd())
  {
    return iter->GetDomain();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::BeginUpdateAnimationValues()
{
  this->ValueIndexMax = -1;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::SetAnimationValue(int index, double value)
{
  vtkSMProperty* property = this->GetAnimatedProperty();
  vtkSMProxy* proxy = this->GetAnimatedProxy();
  if (!proxy || !property)
  {
    vtkErrorMacro("Cue does not have a property set. Cannot animate.");
    return;
  }

  if (vtkSMDomain* domain = this->GetAnimatedDomain())
  {
    // Try to use the domain, if present to animate the property.
    domain->SetAnimationValue(property, index, value);
  }
  else
  {
    // If no domain is available, let use the vtkSMPropertyHelper to simply set
    // the value on the property.
    vtkSMPropertyHelper(property).Set(index, value);
  }

  if (this->ValueIndexMax < index)
  {
    this->ValueIndexMax = index;
  }
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::EndUpdateAnimationValues()
{
  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(this->GetAnimatedProperty());
  if (vp && this->AnimatedElement == -1 && this->ValueIndexMax >= -1)
  {
    vp->SetNumberOfElements(static_cast<unsigned int>(this->ValueIndexMax + 1));
  }
  if (this->AnimatedProxy)
  {
    this->AnimatedProxy->UpdateVTKObjects();
  }

  this->ValueIndexMax = -1;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
