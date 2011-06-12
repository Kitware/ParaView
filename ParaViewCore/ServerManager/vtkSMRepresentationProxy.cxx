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
#include "vtkObjectFactory.h"
#include "vtkPVRepresentedDataInformation.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

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
  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::PostUpdateData()
{
  this->MarkedModified = false;
  this->Superclass::PostUpdateData();
  // PostUpdateData will call InvalidateDataInformation() which will mark
  // RepresentedDataInformationValid as false;
  // this->RepresentedDataInformationValid = false;
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
