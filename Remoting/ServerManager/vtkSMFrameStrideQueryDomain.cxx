// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMFrameStrideQueryDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"

#include <vtkSMPropertyHelper.h>

vtkStandardNewMacro(vtkSMFrameStrideQueryDomain);

//----------------------------------------------------------------------------
vtkSMFrameStrideQueryDomain::vtkSMFrameStrideQueryDomain()
{
  this->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &vtkSMFrameStrideQueryDomain::OnDomainModified);
}

//----------------------------------------------------------------------------
vtkSMFrameStrideQueryDomain::~vtkSMFrameStrideQueryDomain() = default;

//----------------------------------------------------------------------------
void vtkSMFrameStrideQueryDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FrameStride: " << this->FrameStride << endl;
}

//---------------------------------------------------------------------------
int vtkSMFrameStrideQueryDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return 1;
  }

  return vtkSMIntVectorProperty::SafeDownCast(property) != nullptr;
}

//----------------------------------------------------------------------------
void vtkSMFrameStrideQueryDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  auto animationScene =
    vtkSMProxyProperty::SafeDownCast(this->GetRequiredProperty("AnimationScene"));
  if (!animationScene)
  {
    vtkErrorMacro("Missing require property 'AnimationScene'. Update failed.");
    return;
  }
  const int frameStride = vtkSMPropertyHelper(animationScene->GetProxy(0), "Stride").GetAsInt();
  if (frameStride != this->FrameStride)
  {
    this->FrameStride = frameStride;
    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
int vtkSMFrameStrideQueryDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  auto frameStrideProp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (!frameStrideProp)
  {
    vtkErrorMacro("Property is not a vtkSMIntVectorProperty.");
    return 0;
  }
  if (use_unchecked_values)
  {
    frameStrideProp->SetUncheckedElement(0, this->FrameStride);
  }
  else
  {
    frameStrideProp->SetElement(0, this->FrameStride);
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMFrameStrideQueryDomain::OnDomainModified()
{
  vtkSMProperty* prop = this->GetProperty();
  this->SetDefaultValues(prop, true);
  if (prop->GetParent())
  {
    prop->GetParent()->UpdateProperty(prop->GetXMLName());
  }
}
