// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVRepresentationAnimationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

vtkStandardNewMacro(vtkPVRepresentationAnimationHelper);
//----------------------------------------------------------------------------
vtkPVRepresentationAnimationHelper::vtkPVRepresentationAnimationHelper()
{
  this->SourceProxy = nullptr;
}

//----------------------------------------------------------------------------
vtkPVRepresentationAnimationHelper::~vtkPVRepresentationAnimationHelper()
{
  this->SetSourceProxy(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVRepresentationAnimationHelper::SetSourceProxy(vtkSMProxy* proxy)
{
  if (this->SourceProxy != proxy)
  {
    this->SourceProxy = proxy;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentationAnimationHelper::SetVisibility(int visible)
{
  if (!this->SourceProxy)
  {
    return;
  }
  unsigned int numConsumers = this->SourceProxy->GetNumberOfConsumers();
  for (unsigned int cc = 0; cc < numConsumers; cc++)
  {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(this->SourceProxy->GetConsumerProxy(cc));
    if (repr && (repr->GetIsSubProxy() == false))
    {
      auto visibilityProp = repr->GetProperty("Visibility");
      if (visibilityProp && visibilityProp->GetAnimateable())
      {
        vtkSMPropertyHelper(visibilityProp).Set(visible);
        repr->UpdateProperty("Visibility");
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentationAnimationHelper::SetOpacity(double opacity)
{
  if (!this->SourceProxy)
  {
    return;
  }
  unsigned int numConsumers = this->SourceProxy->GetNumberOfConsumers();
  for (unsigned int cc = 0; cc < numConsumers; cc++)
  {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(this->SourceProxy->GetConsumerProxy(cc));
    if (repr && (repr->GetIsSubProxy() == false))
    {
      auto opacityProp = repr->GetProperty("Opacity");
      if (opacityProp && opacityProp->GetAnimateable())
      {
        vtkSMPropertyHelper(opacityProp).Set(opacity);
        repr->UpdateProperty("Opacity");
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentationAnimationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
