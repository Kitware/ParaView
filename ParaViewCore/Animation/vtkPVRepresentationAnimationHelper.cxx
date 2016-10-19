/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentationAnimationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRepresentationAnimationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

vtkStandardNewMacro(vtkPVRepresentationAnimationHelper);
//----------------------------------------------------------------------------
vtkPVRepresentationAnimationHelper::vtkPVRepresentationAnimationHelper()
{
  this->SourceProxy = 0;
}

//----------------------------------------------------------------------------
vtkPVRepresentationAnimationHelper::~vtkPVRepresentationAnimationHelper()
{
  this->SetSourceProxy(0);
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
    if (repr && (repr->GetIsSubProxy() == false) && repr->GetProperty("Visibility"))
    {
      vtkSMPropertyHelper(repr, "Visibility").Set(visible);
      repr->UpdateProperty("Visibility");
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
    if (repr && (repr->GetIsSubProxy() == false) && repr->GetProperty("Opacity"))
    {
      vtkSMPropertyHelper(repr, "Opacity").Set(opacity);
      repr->UpdateProperty("Opacity");
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentationAnimationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
