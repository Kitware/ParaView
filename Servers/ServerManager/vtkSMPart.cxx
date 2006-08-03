/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPart.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPart);
vtkCxxRevisionMacro(vtkSMPart, "1.25");


//----------------------------------------------------------------------------
vtkSMPart::vtkSMPart()
{
  this->SetVTKClassName("vtkDataObject");

  this->ClassNameInformation = vtkPVClassNameInformation::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->ClassNameInformationValid = 0;
  this->DataInformationValid = 0;
  this->PortIndex = 0;

  this->DataObjectProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMPart::~vtkSMPart()
{
  this->ClassNameInformation->Delete();
  this->DataInformation->Delete();
  if (this->DataObjectProxy)
    {
    this->DataObjectProxy->Delete();
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPart::GetDataObjectProxy(int recheck)
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
    this->DataObjectProxy->CreateVTKObjects(0);

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
    this->DataObjectProxy->SetID(0, uid);
    }

  return this->DataObjectProxy;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPart::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation* vtkSMPart::GetClassNameInformation()
{
  if(this->ClassNameInformationValid == 0)
    {
    this->GatherClassNameInformation();
    }
  return this->ClassNameInformation;
}

//----------------------------------------------------------------------------
void vtkSMPart::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
// vtkPVPart used to update before gathering this information ...
void vtkSMPart::GatherDataInformation(int doUpdate)
{
  if (this->GetNumberOfIDs() < 1)
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  pm->GatherInformation(this->ConnectionID, this->Servers, 
    this->DataInformation, this->GetAlgorithmOutputID());

  if (doUpdate)
    {
    this->DataInformationValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSMPart::GatherClassNameInformation()
{
  if (this->GetNumberOfIDs() < 1)
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

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
  this->ClassNameInformationValid = 1;

  // Unassign the id.
  stream << vtkClientServerStream::Delete 
         << uid 
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMPart::InsertExtractPiecesIfNecessary()
{
  if (this->GetNumberOfIDs() < 1)
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  const char* className = this->GetClassNameInformation()->GetVTKClassName();
  vtkClientServerStream stream;
  vtkClientServerID tempDataPiece = {0};
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
    this->GatherDataInformation(0);
    if (this->DataInformation->GetCompositeDataClassName())
      {
      return;
      }
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
    this->GatherDataInformation(0);
    if (this->DataInformation->GetCompositeDataClassName())
      {
      return;
      }
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
  else if (!strcmp(className, "vtkMultiGroupDataSet") ||
           !strcmp(className, "vtkMultiBlockDataSet"))
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
    this->GatherDataInformation(0);
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
      "vtkMultiGroupDataExtractPiece", stream);
    }

  // If no filter is to be inserted, just return.
  if(tempDataPiece.ID == 0)
    {
    return;
    }

  // Connect filter
  stream << vtkClientServerStream::Invoke << tempDataPiece 
         << "SetInputConnection"
         << this->GetAlgorithmOutputID()
         << vtkClientServerStream::End;

  // Release references to old ids
  stream << vtkClientServerStream::Delete 
         << this->GetAlgorithmOutputID()
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
  stream << vtkClientServerStream::Assign << this->GetAlgorithmOutputID()
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
void vtkSMPart::CreateTranslatorIfNecessary()
{
  return;

  if (this->GetNumberOfIDs() < 1)
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  const char* className = this->GetClassNameInformation()->GetVTKClassName();
  if (className == NULL)
    {
    vtkErrorMacro("Missing data information.");
    return;
    }
  vtkClientServerStream stream;
  if (strcmp(className, "vtkImageData") == 0 ||
      strcmp(className, "vtkStructuredPoints") == 0 ||
      strcmp(className, "vtkStructuredGrid") == 0 ||
      strcmp(className, "vtkRectilinearGrid") == 0 )
    {
    // Do not overwrite custom extent translators.
    // PVExtent translator should really be the default,
    // Then we would not need to do this.
    stream << vtkClientServerStream::Invoke
           << this->GetExecutiveID() << "GetExtentTranslator"
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
             << this->GetExecutiveID() << "SetExtentTranslator"
             << translatorID
             << vtkClientServerStream::End;
      // Translator has to be set on source because it is propagated.
      stream << vtkClientServerStream::Invoke
             << this->GetProducerID()
             << "GetOutputDataObject"
             << this->PortIndex
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << translatorID << "SetOriginalSource"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      pm->DeleteStreamObject(translatorID, stream);
      pm->SendStream(this->ConnectionID,
                     this->Servers, 
                     stream);
      }
   }

}

//----------------------------------------------------------------------------
void vtkSMPart::UpdatePipeline()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "UpdateInformation"
         << vtkClientServerStream::End;

  // When getting rid of streaming, throw away the conditional and 
  // keep the else.
  // TODO: disabling for now.
  if (/*vtkPVProcessModule::GetGlobalStreamBlock()*/ 0)
    {
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetPartitionId"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << this->GetExecutiveID() << "SetUpdateExtent" << this->PortIndex
           << vtkClientServerStream::LastResult 
           << pm->GetNumberOfPartitions(this->ConnectionID) * 200 << 0
           << vtkClientServerStream::End; 
    }
  else
    {
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetPartitionId"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << this->GetExecutiveID() << "SetUpdateExtent" << this->PortIndex
           << vtkClientServerStream::LastResult 
           << pm->GetNumberOfPartitions(this->ConnectionID) << 0
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
void vtkSMPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PortIndex: " << this->PortIndex << endl;
}




