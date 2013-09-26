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
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVRepresentedDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

#include <assert.h>

#define MAX_NUMBER_OF_INTERNAL_REPRESENTATIONS 10

vtkStandardNewMacro(vtkSMRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMRepresentationProxy::vtkSMRepresentationProxy()
{
  this->SetExecutiveName("vtkPVDataRepresentationPipeline");
  this->RepresentedDataInformationValid = false;
  this->RepresentedDataInformation = vtkPVRepresentedDataInformation::New();
  this->ProminentValuesInformation = vtkPVProminentValuesInformation::New();
  this->ProminentValuesFraction = -1;
  this->ProminentValuesUncertainty = -1;
  this->MarkedModified = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy::~vtkSMRepresentationProxy()
{
  this->RepresentedDataInformation->Delete();
  this->ProminentValuesInformation->Delete();
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

  // Initialize vtkPVDataRepresentation with a unique ID
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "Initialize"
         << static_cast<unsigned int>(this->GetGlobalID())
         << static_cast<unsigned int>(this->GetGlobalID() + MAX_NUMBER_OF_INTERNAL_REPRESENTATIONS)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

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
  if ((modifiedProxy != this) && this->ObjectsCreated &&
    // this check ensures that for composite representations, we don't end up
    // marking all representations dirty when a sub-representation is modified.
    (this->GetSubProxyName(modifiedProxy) == NULL))
    {
    // We need to check that modified proxy is a type of proxy that affects data
    // rendered/processed by the representation. This is basically a HACK to
    // avoid invalidating geometry when lookuptable and piecewise-function is
    // modified.
    if (!this->MarkedModified && !this->SkipDependency(modifiedProxy))
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
bool vtkSMRepresentationProxy::SkipDependency(vtkSMProxy* producer)
{
  if (producer && producer->GetXMLName() &&
    (strcmp(producer->GetXMLName(), "PVLookupTable") == 0 ||
     strcmp(producer->GetXMLName(), "PiecewiseFunction") == 0))
    {
    return true;
    }

  if (producer && producer->GetXMLGroup() &&
    (strcmp(producer->GetXMLGroup(), "lookup_tables") == 0 ||
     strcmp(producer->GetXMLGroup(), "piecewise_functions") == 0))
    {
    return true;
    }

  return false;
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

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation* vtkSMRepresentationProxy::GetProminentValuesInformation(
  vtkStdString name, int fieldAssoc, int numComponents,
  double uncertaintyAllowed, double fraction)
{
  bool differentAttribute =
    this->ProminentValuesInformation->GetNumberOfComponents() != numComponents ||
    this->ProminentValuesInformation->GetFieldName() != name ||
    strcmp(this->ProminentValuesInformation->GetFieldAssociation(),
      vtkDataObject::GetAssociationTypeAsString(fieldAssoc));
  bool invalid =
    this->ProminentValuesFraction < 0. || this->ProminentValuesUncertainty < 0. ||
    this->ProminentValuesFraction > 1. || this->ProminentValuesUncertainty > 1.;
  bool largerFractionOrLessCertain =
    this->ProminentValuesFraction < fraction ||
    this->ProminentValuesUncertainty > uncertaintyAllowed;
  if (differentAttribute || invalid || largerFractionOrLessCertain)
    {
    vtkTimerLog::MarkStartEvent(
      "vtkSMRepresentationProxy::GetProminentValues");
    this->CreateVTKObjects();
    this->UpdatePipeline();
    // Initialize parameters with specified values:
    this->ProminentValuesInformation->Initialize();
    this->ProminentValuesInformation->SetFieldAssociation(
      vtkDataObject::GetAssociationTypeAsString(fieldAssoc));
    this->ProminentValuesInformation->SetFieldName(name);
    this->ProminentValuesInformation->SetNumberOfComponents(numComponents);
    this->ProminentValuesInformation->SetUncertainty(uncertaintyAllowed);
    this->ProminentValuesInformation->SetFraction(fraction);

    // Ask the server to fill out the rest of the information:
    this->GatherInformation(this->ProminentValuesInformation);
    vtkTimerLog::MarkEndEvent(
      "vtkSMRepresentationProxy::GetProminentValues");
    this->ProminentValuesFraction = fraction;
    this->ProminentValuesUncertainty = uncertaintyAllowed;
    }

  return this->ProminentValuesInformation;
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
//---------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRepresentationProxy::GetGlobalID()
{
  bool has_gid = this->HasGlobalID();

  if (!has_gid && this->Session != NULL)
    {
    // reserve 1+MAX_NUMBER_OF_INTERNAL_REPRESENTATIONS contiguous IDs for the source proxies and possible extract
    // selection proxies.
    this->SetGlobalID(
      this->GetSession()->GetNextChunkGlobalUniqueIdentifier(1 +
        MAX_NUMBER_OF_INTERNAL_REPRESENTATIONS));
    }
  return this->GlobalID;
}

//---------------------------------------------------------------------------
class vtkSMRepresentationProxyObserver : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMProperty> Output;
  typedef vtkCommand Superclass;
  virtual const char* GetClassNameInternal() const
    { return "vtkSMRepresentationProxyObserver"; }
  static vtkSMRepresentationProxyObserver* New()
    {
    return new vtkSMRepresentationProxyObserver();
    }
  virtual void Execute(vtkObject* caller, unsigned long event, void* calldata)
    {
    vtkSMProperty* input = vtkSMProperty::SafeDownCast(caller);
    if (input && this->Output)
      {
      // this will copy both checked and unchecked property values.
      this->Output->Copy(input);
      }
    }
};

//---------------------------------------------------------------------------
void vtkSMRepresentationProxy::LinkProperty(
  vtkSMProperty* input, vtkSMProperty* output)
{
  if (input == output || input == NULL || output == NULL)
    {
    vtkErrorMacro("Invalid call to LinkProperty. Check arguments.");
    return;
    }

  vtkSMRepresentationProxyObserver* observer =
    vtkSMRepresentationProxyObserver::New();
  observer->Output = output;
  input->AddObserver(vtkCommand::PropertyModifiedEvent, observer);
  input->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, observer);
  observer->FastDelete();
}
