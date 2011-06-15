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
#include "vtkSMRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVRepresentedDataInformation.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMRepresentationProxy::vtkSMRepresentationProxy()
{
  this->SetExecutiveName("vtkPVDataRepresentationPipeline");
  this->RepresentedDataInformationValid = false;
  this->RepresentedDataInformation = vtkPVRepresentedDataInformation::New();
  this->MarkedModified = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy::~vtkSMRepresentationProxy()
{
  this->RepresentedDataInformation->Delete();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to add listeners...
  if(this->Location == 0 || !this->ObjectsCreated)
    {
    return;
    }

  vtkObject::SafeDownCast(this->GetClientSideObject())->AddObserver(
    vtkCommand::UpdateDataEvent,
    this, &vtkSMRepresentationProxy::OnVTKRepresentationUpdated);
}

//---------------------------------------------------------------------------
int vtkSMRepresentationProxy::LoadXMLState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  vtkTypeUInt32 oldserver = this->Location;
  int ret = this->Superclass::LoadXMLState(proxyElement, locator);
  this->Location = oldserver;
  return ret;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  this->Superclass::AddConsumer(property, proxy);
  for (unsigned int cc=0; cc < this->GetNumberOfSubProxies(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->GetSubProxy(cc));
    if (repr)
      {
      repr->AddConsumer(property, proxy);
      }
    }

}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  this->Superclass::RemoveConsumer(property, proxy);
  for (unsigned int cc=0; cc < this->GetNumberOfSubProxies(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->GetSubProxy(cc));
    if (repr)
      {
      repr->RemoveConsumer(property, proxy);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::RemoveAllConsumers()
{
  this->Superclass::RemoveAllConsumers();
  for (unsigned int cc=0; cc < this->GetNumberOfSubProxies(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->GetSubProxy(cc));
    if (repr)
      {
      repr->RemoveAllConsumers();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::UpdatePipeline()
{
  if (!this->NeedsUpdate)
    {
    return;
    }

  this->UpdatePipelineInternal(0, false);
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::UpdatePipeline(double time)
{
  this->UpdatePipelineInternal(time, true);
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::UpdatePipelineInternal(
  double time, bool doTime)
{
  vtkClientServerStream stream;
  if (doTime)
    {
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "SetUpdateTime" << time
           << vtkClientServerStream::End;
    }

  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "Update"
         << vtkClientServerStream::End;

  this->GetSession()->PrepareProgress();
  this->ExecuteStream(stream);
  this->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if ((modifiedProxy != this) && this->ObjectsCreated)
    {
    if (!this->MarkedModified)
      {
      this->MarkedModified = true;
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "MarkModified"
         << vtkClientServerStream::End;
      this->ExecuteStream(stream);
      }
    }

  // vtkSMProxy::MarkDirty does not call MarkConsumersAsDirty unless
  // this->NeedsUpdate is false. Generally, that's indeed correct since we we
  // have marked the consumer dirty previously, we don't need to do it again.
  // However since consumers of representations are generally views, they need
  // to marked dirty everytime (otherwise unhiding a representation would not
  // result in the view realizing that data may have changed). Hence we force
  // NeedsUpdate to false.
  this->NeedsUpdate = false;

  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::OnVTKRepresentationUpdated()
{
  this->MarkedModified = false;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::ViewUpdated(vtkSMProxy* view)
{
  if (this->MarkedModified == false)
    {
    this->PostUpdateData();
    }

  // If this class has sub-representations, we need to tell those that the view
  // has updated as well.
  for (unsigned int cc=0; cc < this->GetNumberOfSubProxies(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      this->GetSubProxy(cc));
    if (repr)
      {
      repr->ViewUpdated(view);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::PostUpdateData()
{
  // PostUpdateData may get called on all representations on the client side
  // whenever the view updates. However, the underlying vtkPVDataRepresentation
  // object may not have updated (possibly because of visibility being false).
  // In that case, we should not let PostUpdateData() happen. The following
  // check ensures that PostUpdateData() call has any effect only after the VTK
  // representation has updated as well.
  if (this->MarkedModified == false)
    {
    this->Superclass::PostUpdateData();
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::InvalidateDataInformation()
{
  this->Superclass::InvalidateDataInformation();
  this->RepresentedDataInformationValid = false;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationProxy::GetRepresentedDataInformation()
{
  if (!this->RepresentedDataInformationValid)
    {
    vtkTimerLog::MarkStartEvent(
      "vtkSMRepresentationProxy::GetRepresentedDataInformation");
    this->RepresentedDataInformation->Initialize();
    this->GatherInformation(this->RepresentedDataInformation);
    vtkTimerLog::MarkEndEvent(
      "vtkSMRepresentationProxy::GetRepresentedDataInformation");
    this->RepresentedDataInformationValid = true;
    }

  return this->RepresentedDataInformation;
}

//-----------------------------------------------------------------------------
void vtkSMRepresentationProxy::ViewTimeChanged()
{
  vtkSMProxy* current = this;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    current->GetProperty("Input"));
  while (current && pp && pp->GetNumberOfProxies() > 0)
    {
    current = pp->GetProxy(0);
    pp = vtkSMProxyProperty::SafeDownCast(current->GetProperty("Input"));
    }

  if (current)
    {
    current->MarkModified(current);
    }
}
//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
