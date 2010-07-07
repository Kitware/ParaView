/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOutputPort.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSourceProxy.h"
#include "vtkTimerLog.h"

#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMOutputPort);


//----------------------------------------------------------------------------
vtkSMOutputPort::vtkSMOutputPort()
{
  this->SetVTKClassName("vtkDataObject");

  this->ClassNameInformation = vtkPVClassNameInformation::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->TemporalDataInformation = vtkPVTemporalDataInformation::New();
  this->ClassNameInformationValid = 0;
  this->DataInformationValid = false;
  this->TemporalDataInformationValid = false;
  this->PortIndex = 0;
  this->SourceProxy = 0;
  this->DataObjectProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMOutputPort::~vtkSMOutputPort()
{
  this->SetSourceProxy(0);
  this->ClassNameInformation->Delete();
  this->DataInformation->Delete();
  this->TemporalDataInformation->Delete();
  if (this->DataObjectProxy)
    {
    this->DataObjectProxy->Delete();
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!this->ProducerID.IsNull() && pm)
    {
    vtkClientServerStream stream;
    pm->DeleteStreamObject(this->ProducerID, stream);
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  if (!this->ExecutiveID.IsNull() && pm)
    {
    vtkClientServerStream stream;
    pm->DeleteStreamObject(this->ExecutiveID, stream);
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::InitializeWithIDs(vtkClientServerID outputID, 
                                  vtkClientServerID producerID, 
                                  vtkClientServerID executiveID)
{
  if (this->ObjectsCreated || outputID.IsNull() || producerID.IsNull() ||
      executiveID.IsNull())
    {
    return;
    }
  this->ObjectsCreated = 1;
  this->GetSelfID(); // this will ensure that the SelfID is assigned properly.
  this->VTKObjectID = outputID;
  this->ProducerID = producerID;
  this->ExecutiveID = executiveID;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMOutputPort::SaveRevivalState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* revivalElem = this->Superclass::SaveRevivalState(root);
  if (revivalElem && this->ObjectsCreated)
    {
    vtkPVXMLElement* elem = vtkPVXMLElement::New();
    elem->SetName("ProducerID");
    elem->AddAttribute("id", static_cast<unsigned int>(this->ProducerID.ID));
    revivalElem->AddNestedElement(elem);
    elem->Delete();

    elem = vtkPVXMLElement::New();
    elem->SetName("ExecutiveID");
    elem->AddAttribute("id", static_cast<unsigned int>(this->ExecutiveID.ID));
    revivalElem->AddNestedElement(elem);
    elem->Delete();
    }

  return revivalElem;
}

//----------------------------------------------------------------------------
int vtkSMOutputPort::LoadRevivalState(vtkPVXMLElement* revivalElem)
{
  if (!this->Superclass::LoadRevivalState(revivalElem))
    {
    return 0;
    }

  unsigned int num_elems = revivalElem->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc <num_elems; cc++)
    {
    vtkPVXMLElement* elem = revivalElem->GetNestedElement(cc);
    if (elem && elem->GetName())
      {
      if (strcmp(elem->GetName(), "ProducerID") == 0)
        {
        vtkClientServerID id;
        int int_id;
        if (elem->GetScalarAttribute("id", &int_id) && int_id)
          {
          this->ProducerID.ID = int_id;
          }
        }
      else if (strcmp(elem->GetName(), "ExecutiveID") == 0)
        {
        vtkClientServerID id;
        int int_id;
        if (elem->GetScalarAttribute("id", &int_id) && int_id)
          {
          this->ExecutiveID.ID = int_id;
          }
        }
      }
    }
  if (this->ProducerID.IsNull() || this->ExecutiveID.IsNull())
    {
    vtkErrorMacro("Missing producer or executive ID.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMOutputPort::GetDataObjectProxy(int recheck)
{
  if (!this->DataObjectProxy)
    {
    recheck = 1;
    }
  if (recheck)
    {
    if (this->DataObjectProxy)
      {
      this->DataObjectProxy->Delete();
      }
    this->DataObjectProxy = vtkSMProxy::New();
    this->DataObjectProxy->SetConnectionID(this->ConnectionID);
    this->DataObjectProxy->SetServers(this->Servers);

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetProducerID()
           << "GetOutputDataObject"
           << this->PortIndex
           << vtkClientServerStream::End;
    vtkClientServerID uid = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign 
           << uid << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    this->DataObjectProxy->InitializeWithID(uid);
    }

  return this->DataObjectProxy;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataInformation()
{
  if (!this->DataInformationValid)
    {
    vtksys_ios::ostringstream mystr;
    mystr << this->GetSourceProxy()->GetXMLName() << "::GatherInformation";
    vtkTimerLog::MarkStartEvent(mystr.str().c_str());
    this->GatherDataInformation();
    vtkTimerLog::MarkEndEvent(mystr.str().c_str());
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation* vtkSMOutputPort::GetTemporalDataInformation()
{
  if (!this->TemporalDataInformationValid)
    {
    this->GatherTemporalDataInformation();
    }
  return this->TemporalDataInformation;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation* vtkSMOutputPort::GetClassNameInformation()
{
  if(this->ClassNameInformationValid == 0)
    {
    this->GatherClassNameInformation();
    }
  return this->ClassNameInformation;
}

//----------------------------------------------------------------------------
const char* vtkSMOutputPort::GetDataClassName()
{
  return this->GetClassNameInformation()->GetVTKClassName();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::InvalidateDataInformation()
{
  this->DataInformationValid = false;
  this->ClassNameInformationValid = false;
  this->TemporalDataInformationValid = false;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherDataInformation()
{
  if (this->GetID().IsNull())
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);
  this->DataInformation->Initialize();
  pm->GatherInformation(this->ConnectionID, this->Servers, 
                        this->DataInformation, this->GetID());

  this->DataInformationValid = true;

  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherTemporalDataInformation()
{
  if (this->GetID().IsNull())
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);
  this->TemporalDataInformation->Initialize();
  pm->GatherInformation(this->ConnectionID, this->Servers, 
                        this->TemporalDataInformation, this->GetID());

  this->TemporalDataInformationValid = true;
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherClassNameInformation()
{
  if (this->GetID().IsNull())
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkObjectBase* cso = this->GetSourceProxy()->GetClientSideObject();
  if (cso)
    {
    this->ClassNameInformation->CopyFromObject(
      vtkAlgorithm::SafeDownCast(cso)->GetOutputDataObject(
        this->PortIndex));
    }
  else
    {
    // Temporarily assign an id to the output object so that we
    // can obtain it's name.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->GetProducerID()
           << "GetOutputDataObject"
           << this->PortIndex
           << vtkClientServerStream::End;
    vtkClientServerID uid = pm->GetUniqueID();
    stream << vtkClientServerStream::Assign 
           << uid << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    pm->SendStream(this->ConnectionID, this->Servers, stream);

    pm->GatherInformation(this->ConnectionID, 
                          this->Servers,
                          this->ClassNameInformation, 
                          uid);

    // Unassign the id.
    stream << vtkClientServerStream::Delete 
           << uid 
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  this->ClassNameInformationValid = 1;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::InsertExtractPiecesIfNecessary()
{
  if (this->GetID().IsNull())
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  const char* className = this->GetClassNameInformation()->GetVTKClassName();
  vtkClientServerStream stream;
  vtkClientServerID tempDataPiece;
  if (className == NULL)
    {
    vtkErrorMacro("Missing data information.");
    return;
    }
  if (!strcmp(className, "vtkPolyData"))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions(this->ConnectionID) == 1)
      {
      return;
      }  
    stream << vtkClientServerStream::Invoke 
           << this->GetProducerID() << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    stream << vtkClientServerStream::Invoke 
           << this->GetExecutiveID() 
           << "GetMaximumNumberOfPieces"
           << this->PortIndex
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    int num =0;
    pm->GetLastResult(this->ConnectionID, 
             vtkProcessModule::GetRootId(this->Servers)).GetArgument(0,0,&num);
    if (num != 1)
      { // The source can already produce pieces.
      return;
      }
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      tempDataPiece = pm->NewStreamObject("vtkExtractPolyDataPiece", stream);
      }
    else
      {
      tempDataPiece = pm->NewStreamObject("vtkTransmitPolyDataPiece", stream);
// TODO: Add observers. Move logging to process object
//       vtkClientServerStream start;
//       start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//             << "LogStartEvent" << "Execute TransmitPData" 
//             << vtkClientServerStream::End;
//       stream << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "StartEvent"
//                       << start
//                       << vtkClientServerStream::End;
//       vtkClientServerStream end;
//       end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//           << "LogEndEvent" << "Execute TransmitPData" 
//           << vtkClientServerStream::End;
//       stream << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "EndEvent"
//                       << end
//                       << vtkClientServerStream::End;
      }
    }
  else if (!strcmp(className, "vtkUnstructuredGrid"))
    {
    // Don't add anything if we are only using one processes.
    // Image can still benifit from its cache so this check
    // is specific for unstructured data.
    if (pm->GetNumberOfPartitions(this->ConnectionID) == 1)
      {
      return;
      }
    stream << vtkClientServerStream::Invoke 
           << this->GetProducerID() << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   this->Servers, 
                   stream);
    stream << vtkClientServerStream::Invoke 
           << this->GetExecutiveID() 
           << "GetMaximumNumberOfPieces"
           << this->PortIndex
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
                   vtkProcessModule::GetRootId(this->Servers),
                   stream);
    int num =0;
    pm->GetLastResult(this->ConnectionID, 
           vtkProcessModule::GetRootId(this->Servers)).GetArgument(0,0,&num);
    if (num != 1)
      { // The source can already produce pieces.
      return;
      }

    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      { 
      tempDataPiece = pm->NewStreamObject(
        "vtkExtractUnstructuredGridPiece", stream);
      }
    else
      {
      tempDataPiece = pm->NewStreamObject(
        "vtkTransmitUnstructuredGridPiece", stream);
// TODO: Add observers. Move logging to process object
//       vtkClientServerStream start;
//       start << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//             << "LogStartEvent" << "Execute TransmitPData" 
//             << vtkClientServerStream::End;
//       stream << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "StartEvent"
//                       << start
//                       << vtkClientServerStream::End;
//       vtkClientServerStream end;
//       end << vtkClientServerStream::Invoke << pm->GetApplicationID() 
//           << "LogEndEvent" << "Execute TransmitPData" 
//           << vtkClientServerStream::End;
//       stream << vtkClientServerStream::Invoke << tempDataPiece 
//                       << "AddObserver"
//                       << "EndEvent"
//                       << end
//                       << vtkClientServerStream::End;
      }
    }
  else if (strcmp(className, "vtkHierarchicalBoxDataSet") == 0 ||
           strcmp(className, "vtkMultiBlockDataSet") == 0)
    {
    if (pm->GetNumberOfPartitions(this->ConnectionID) == 1)
      {
      // We're only operating with one processor, so it should have
      // all the data.
      return;
      }
    stream << vtkClientServerStream::Invoke 
           << this->GetProducerID() << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   this->Servers, 
                   stream);
    stream << vtkClientServerStream::Invoke
           << this->GetExecutiveID() 
           << "GetMaximumNumberOfPieces"
           << this->PortIndex
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   this->Servers, 
                   stream);
    int num = 0;
    pm->GetLastResult(this->ConnectionID, 
         vtkProcessModule::GetRootId(this->Servers)).GetArgument(0,0,&num);
    if (num != 1)
      { // The source can already produce pieces.
      return;
      }

    tempDataPiece = pm->NewStreamObject(
      "vtkExtractPiece", stream);
    }

  // If no filter is to be inserted, just return.
  if(tempDataPiece.ID == 0)
    {
    return;
    }

  // Set the right executive
  vtkClientServerID execId = pm->NewStreamObject(
    "vtkCompositeDataPipeline", stream);
  stream << vtkClientServerStream::Invoke 
         << tempDataPiece << "SetExecutive" << execId 
         << vtkClientServerStream::End;
  pm->DeleteStreamObject(execId, stream);
  
  // Connect filter
  stream << vtkClientServerStream::Invoke << tempDataPiece 
         << "SetInputConnection"
         << this->GetID()
         << vtkClientServerStream::End;

  // Release references to old ids
  stream << vtkClientServerStream::Delete 
         << this->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Delete 
         << this->GetProducerID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Delete 
         << this->GetExecutiveID()
         << vtkClientServerStream::End;

  // Reassign ids to the new output filter
  stream << vtkClientServerStream::Invoke << tempDataPiece 
         << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign << this->GetID()
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << tempDataPiece 
         << "GetExecutive" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign << this->GetExecutiveID()
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Assign << this->GetProducerID()
         << tempDataPiece
         << vtkClientServerStream::End;

  // Port index is now 0
  this->PortIndex = 0;

  // We don't have to keep reference to the filter
  pm->DeleteStreamObject(tempDataPiece, stream);

  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
// Create the extent translator (sources with no inputs only).
// Needs to be before "ExtractPieces" because translator propagates.
void vtkSMOutputPort::CreateTranslatorIfNecessary()
{
  if (this->GetID().IsNull())
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  vtkClientServerStream stream;
  // Do not overwrite custom extent translators.
  // PVExtent translator should really be the default,
  // Then we would not need to do this.
  stream << vtkClientServerStream::Invoke
         << this->GetExecutiveID() 
         << "GetExtentTranslator" 
         << this->PortIndex
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "GetClassName"
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 vtkProcessModule::GetRootId(this->Servers),
                 stream);
  char* classname = 0;
  if(!pm->GetLastResult(this->ConnectionID,
    vtkProcessModule::GetRootId(this->Servers)).GetArgument(0,0,&classname))
    {
    vtkErrorMacro(<< "Faild to get server result.");
    }
  if(classname && strcmp(classname, "vtkExtentTranslator") == 0)
    {
    vtkClientServerID translatorID =
      pm->NewStreamObject("vtkPVExtentTranslator", stream);
    stream << vtkClientServerStream::Invoke
           << this->GetExecutiveID() 
           << "SetExtentTranslator" 
           << this->PortIndex
           << translatorID
           << vtkClientServerStream::End;
    // Translator has to be set on source because it is propagated.
    stream << vtkClientServerStream::Invoke
           << translatorID << "SetOriginalSource"
           << this->GetProducerID()
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << translatorID << "SetPortIndex"
           << this->PortIndex
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(translatorID, stream);
    pm->SendStream(this->ConnectionID,
                   this->Servers, 
                   stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::UpdatePipeline()
{
  this->UpdatePipelineInternal(0.0, false);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::UpdatePipeline(double time)
{
  this->UpdatePipelineInternal(time, true);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::UpdatePipelineInternal(double time, 
                                             bool doTime)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "UpdateInformation"
         << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->GetExecutiveID() << "SetUpdateExtent" << this->PortIndex
         << vtkClientServerStream::LastResult 
         << pm->GetNumberOfPartitions(this->ConnectionID) << 0
         << vtkClientServerStream::End; 

  if (doTime)
    {
    stream << vtkClientServerStream::Invoke
           << this->GetExecutiveID() << "SetUpdateTimeStep" 
           << this->PortIndex << time
           << vtkClientServerStream::End;
    }

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "Update"
         << vtkClientServerStream::End;

  pm->SendPrepareProgress(this->ConnectionID);
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PortIndex: " << this->PortIndex << endl;
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
}




