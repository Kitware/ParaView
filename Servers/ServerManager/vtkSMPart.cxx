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
vtkCxxRevisionMacro(vtkSMPart, "1.24");


//----------------------------------------------------------------------------
vtkSMPart::vtkSMPart()
{
  this->SetVTKClassName("vtkDataObject");

  this->ClassNameInformation = vtkPVClassNameInformation::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->ClassNameInformationValid = 0;
  this->DataInformationValid = 0;
  this->UpdateNeeded = 1;
}

//----------------------------------------------------------------------------
vtkSMPart::~vtkSMPart()
{
  this->ClassNameInformation->Delete();
  this->DataInformation->Delete();
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

  // The vtkSMSourceProxy updates the pipeline before gathering
  // information. The update is no longer necessary here.
//   if (doUpdate)
//     {
//     pm->SendPrepareProgress(this->ConnectionID);
//     this->Update();
//     pm->SendCleanupPendingProgress(this->ConnectionID);
//     }

  pm->GatherInformation(this->ConnectionID, vtkProcessModule::DATA_SERVER, 
    this->DataInformation, this->GetID(0));

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
  pm->GatherInformation(this->ConnectionID, vtkProcessModule::DATA_SERVER,
    this->ClassNameInformation, this->GetID(0));
  this->ClassNameInformationValid = 1;
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
           << this->GetID(0) << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::DATA_SERVER, stream);
    this->GatherDataInformation(0);
    if (this->DataInformation->GetCompositeDataClassName())
      {
      return;
      }
    stream << vtkClientServerStream::Invoke 
                    << this->GetID(0) << "GetMaximumNumberOfPieces"
                    << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    int num =0;
    pm->GetLastResult(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0,0,&num);
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
           << this->GetID(0) << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER, stream);
    this->GatherDataInformation(0);
    if (this->DataInformation->GetCompositeDataClassName())
      {
      return;
      }
    stream << vtkClientServerStream::Invoke 
                    << this->GetID(0) << "GetMaximumNumberOfPieces"
                    << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    int num =0;
    pm->GetLastResult(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0,0,&num);
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
           << this->GetID(0) << "UpdateInformation"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER, stream);
    this->GatherDataInformation(0);
    stream << vtkClientServerStream::Invoke
           << this->GetID(0) << "GetMaximumNumberOfPieces"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    int num = 0;
    pm->GetLastResult(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0,0,&num);
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

  // Connect the filter to the pipeline.  
  if (!this->DataInformation->GetCompositeDataClassName())
    {
    stream << vtkClientServerStream::Invoke << tempDataPiece 
           << "SetInput"
           << this->GetID(0)
           << vtkClientServerStream::End;
    }
  else
    {
    stream << vtkClientServerStream::Invoke
           << this->GetID(0)
           << "GetProducerPort"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "GetProducer"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "GetExecutive"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "GetCompositeOutputData" << 0
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << tempDataPiece
           << "SetInput" << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  stream << vtkClientServerStream::Invoke << tempDataPiece 
                  << "GetOutput"
                  << vtkClientServerStream::End;
  this->SetID(0, pm->GetUniqueID());
  stream << vtkClientServerStream::Assign << this->GetID(0)
                  << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->DeleteStreamObject(tempDataPiece, stream);
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::DATA_SERVER, stream);
}

//----------------------------------------------------------------------------
// Create the extent translator (sources with no inputs only).
// Needs to be before "ExtractPieces" because translator propagates.
void vtkSMPart::CreateTranslatorIfNecessary()
{
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
           << this->GetID(0) << "GetExtentTranslator"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << vtkClientServerStream::LastResult
           << "GetClassName"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    char* classname = 0;
    if(!pm->GetLastResult(this->ConnectionID,
        vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0,0,&classname))
      {
      vtkErrorMacro(<< "Faild to get server result.");
      }
    if(classname && strcmp(classname, "vtkExtentTranslator") == 0)
      {
      vtkClientServerID translatorID =
        pm->NewStreamObject("vtkPVExtentTranslator", stream);
      stream << vtkClientServerStream::Invoke
             << this->GetID(0) << "SetExtentTranslator"
             << translatorID
             << vtkClientServerStream::End;
      // Translator has to be set on source because it is propagated.
      stream << vtkClientServerStream::Invoke
             << translatorID << "SetOriginalSource"
             << this->GetID(0)
             << vtkClientServerStream::End;
      pm->DeleteStreamObject(translatorID, stream);
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::DATA_SERVER, stream);
      }
   }

}

//----------------------------------------------------------------------------
void vtkSMPart::Update()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetID(0) << "UpdateInformation"
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
           << this->GetID(0) << "SetUpdateExtent"
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
           << this->GetID(0) << "SetUpdateExtent"
           << vtkClientServerStream::LastResult 
           << pm->GetNumberOfPartitions(this->ConnectionID) << 0
           << vtkClientServerStream::End; 
    }   
  stream << vtkClientServerStream::Invoke 
         << this->GetID(0) << "Update"
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::DATA_SERVER, stream);
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//----------------------------------------------------------------------------
void vtkSMPart::MarkForUpdate()
{
  this->UpdateNeeded = 1;
}

//----------------------------------------------------------------------------
void vtkSMPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}




