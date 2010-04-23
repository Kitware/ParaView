/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationAnimationHelperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentationAnimationHelperProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMRepresentationAnimationHelperProxy);
//----------------------------------------------------------------------------
vtkSMRepresentationAnimationHelperProxy::vtkSMRepresentationAnimationHelperProxy()
{
  this->SourceProxy  = 0;
}

//----------------------------------------------------------------------------
vtkSMRepresentationAnimationHelperProxy::~vtkSMRepresentationAnimationHelperProxy()
{
  this->SetSourceProxy(0);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationAnimationHelperProxy::SetSourceProxy(vtkSMProxy* proxy)
{
  if (this->SourceProxy != proxy)
    {
    this->SourceProxy = proxy;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationAnimationHelperProxy::SetVisibility(int visible)
{
  if (!this->SourceProxy)
    {
    return;
    }
  unsigned int numConsumers = this->SourceProxy->GetNumberOfConsumers();
  for (unsigned int cc=0; cc < numConsumers; cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->SourceProxy->GetConsumerProxy(cc));
    if (repr && repr->GetProperty("Visibility"))
      {
      vtkSMPropertyHelper(repr, "Visibility").Set(visible);
      repr->UpdateProperty("Visibility");
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationAnimationHelperProxy::SetOpacity(double opacity)
{
  if (!this->SourceProxy)
    {
    return;
    }
  unsigned int numConsumers = this->SourceProxy->GetNumberOfConsumers();
  for (unsigned int cc=0; cc < numConsumers; cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->SourceProxy->GetConsumerProxy(cc));
    if (repr && repr->GetProperty("Opacity"))
      {
      vtkSMPropertyHelper(repr, "Opacity").Set(opacity);
      repr->UpdateProperty("Opacity");
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationAnimationHelperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


