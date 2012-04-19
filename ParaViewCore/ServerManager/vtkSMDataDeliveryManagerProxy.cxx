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
#include "vtkSMDataDeliveryManagerProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMDataDeliveryManagerProxy);
//----------------------------------------------------------------------------
vtkSMDataDeliveryManagerProxy::vtkSMDataDeliveryManagerProxy()
  : UpdateObserverTag(0)
{
}

//----------------------------------------------------------------------------
vtkSMDataDeliveryManagerProxy::~vtkSMDataDeliveryManagerProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManagerProxy::SetViewProxy(vtkSMViewProxy* viewproxy)
{
  if (viewproxy)
    {
    this->CreateVTKObjects();
    }

  if (!this->ObjectsCreated || viewproxy == this->ViewProxy)
    {
    return;
    }

  if (this->ViewProxy && this->UpdateObserverTag)
    {
    this->ViewProxy->RemoveObserver(this->UpdateObserverTag);
    this->UpdateObserverTag = 0;
    }
  this->ViewProxy = viewproxy;
  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << VTKOBJECT(this)
      << "SetView"
      << VTKOBJECT(viewproxy)
      << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  if (this->ViewProxy)
    {
    this->UpdateObserverTag = this->ViewProxy->AddObserver(
      vtkCommand::UpdateDataEvent,
      this, &vtkSMDataDeliveryManagerProxy::OnViewUpdated);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManagerProxy::OnViewUpdated()
{
  vtkPVDataDeliveryManager::SafeDownCast(this->GetClientSideObject())->OnViewUpdated();
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManagerProxy::Deliver(bool interactive)
{
  if (this->ObjectsCreated)
    {
    vtkPVDataDeliveryManager::SafeDownCast(
      this->GetClientSideObject())->Deliver(this, interactive);
    }
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMDataDeliveryManagerProxy::GetViewProxy()
{
  return this->ViewProxy;
}

//----------------------------------------------------------------------------
void vtkSMDataDeliveryManagerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
