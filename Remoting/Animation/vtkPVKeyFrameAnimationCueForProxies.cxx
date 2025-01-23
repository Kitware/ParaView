// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVKeyFrameAnimationCueForProxies.h"

#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

#include <algorithm>

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
  this->SetAnimatedProxy(nullptr);
  this->SetAnimatedPropertyName(nullptr);
  this->SetAnimatedDomainName(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCueForProxies::RemoveAnimatedProxy()
{
  this->SetAnimatedProxy(nullptr);
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

  this->ValueIndexMax = std::max(this->ValueIndexMax, index);
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
