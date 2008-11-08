/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingOutputPort.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingOutputPort.h"

#include "vtkSMProxyManager.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMStreamingHelperProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMStreamingOutputPort);
vtkCxxRevisionMacro(vtkSMStreamingOutputPort, "1.1");

//----------------------------------------------------------------------------
vtkSMStreamingOutputPort::vtkSMStreamingOutputPort()
{ 
  /*
  vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
  this->Helper = vtkSMStreamingHelperProxy::SafeDownCast(
    pxm->GetProxy("helpers", vtkSMStreamingHelperProxy::GetInstanceName()));
  */
  this->Helper = NULL;
}

//----------------------------------------------------------------------------
vtkSMStreamingOutputPort::~vtkSMStreamingOutputPort()
{
/*
  if (this->Helper)
    {
    this->Helper->Delete();
    }
*/
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMStreamingOutputPort::GetDataInformation()
{
  if (!this->DataInformationValid)
    {
    //cerr << this << " Recompute Info" << endl;
    this->GatherDataInformation();
    }
  else
    {
    //cerr << this << " Reuse Info" << endl;
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkSMStreamingOutputPort::InvalidateDataInformation()
{
//  cerr << this << " Invalidate Info" << endl;
  this->DataInformationValid = false;
  this->ClassNameInformationValid = false;
}


//----------------------------------------------------------------------------
// vtkPVPart used to update before gathering this information ...
void vtkSMStreamingOutputPort::GatherDataInformation(int vtkNotUsed(doUpdate))
{
//cerr << "void vtkSMStreamingOutputPort(" << this << ")::GatherDataInformation(" << doUpdate <<")" << endl;

  if (this->GetID().IsNull())
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);
  this->DataInformation->Initialize();

  ///
  vtkPVDataInformation *di = vtkPVDataInformation::New();
  vtkClientServerStream stream;

  int nPasses = 1;
  int doPrints = 0;
  if (this->Helper)
    {
    //nPasses = this->Helper->GetStreamedPasses();
    //helperdoPrints = this->Helper->GetEnableStreamMessages();
    }

  // Use nPasses from proxy manager for now
  nPasses = this->GetProxyManager()->GetStreamedPasses();

//  cerr << "SMOP::" << this << " GatherData ";
  vtkClientServerID infoHelper = pm->NewStreamObject("vtkPriorityHelper", stream);
  //commented out to make piece cache appendpolydata work
  //if (!this->GetProxyManager()->GetUseCulling())
//    {
//    stream 
//      << vtkClientServerStream::Invoke 
//      << infoHelper << "EnableCullingOff"
//      << vtkClientServerStream::End;
//    }

  for (int i = 0; i < 1; i++)
    {
    if (doPrints)
      {
      cerr << "SMOP::" << this << " Conditionally GatherData " << i << endl;
      stream 
        << vtkClientServerStream::Invoke 
        << infoHelper << "EnableStreamMessagesOn"
        << vtkClientServerStream::End;
      }
    //tell it to work on next piece
    stream 
      << vtkClientServerStream::Invoke 
      << infoHelper << "SetInputConnection" << this->GetID()
      << vtkClientServerStream::End;

    stream 
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End;

    stream
      << vtkClientServerStream::Invoke
      << infoHelper 
      << "SetSplitUpdateExtent" 
      << this->PortIndex
      << vtkClientServerStream::LastResult 
      << i
      << pm->GetNumberOfPartitions(this->ConnectionID)
      << nPasses
      << 0
      << 1
      << vtkClientServerStream::End; 

    //if there is something there (priority not 0 on next piece) 
    //this will update so that we get the rest of the information about the next piece
    stream 
      << vtkClientServerStream::Invoke 
      << infoHelper << "ConditionallyUpdate"
      << vtkClientServerStream::End;

    pm->SendStream(this->ConnectionID, this->Servers, stream);
    
    //gather the data information for that piece
    di->Initialize();
    pm->GatherInformation(this->ConnectionID, this->Servers, 
                          di, 
                          infoHelper);
    
    //merge it in
    this->DataInformation->AddInformation(di);
    }

//  cerr << endl;
  di->Delete();
  pm->DeleteStreamObject(infoHelper, stream);

  this->DataInformationValid = true;

  pm->SendCleanupPendingProgress(this->ConnectionID);
}



//----------------------------------------------------------------------------
void vtkSMStreamingOutputPort::UpdatePipelineInternal(double time, bool doTime)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "UpdateInformation"
         << vtkClientServerStream::End;

  int nPasses = 1;
  int doPrints = 0;
  if (this->Helper)
    {
    //nPasses = this->Helper->GetStreamedPasses();
    //doPrints = this->Helper->GetEnableStreamMessages();
    }

  // Use nPasses from proxy manager for now
  nPasses = this->GetProxyManager()->GetStreamedPasses();

//  cerr << "SMOP::" << this << " TUpdate ";
  vtkClientServerID infoHelper = pm->NewStreamObject("vtkPriorityHelper", stream);
//  stream 
//    << vtkClientServerStream::Invoke 
//    << infoHelper << "EnableCullingOff"
//    << vtkClientServerStream::End;

  // Get the first piece for meta-data (DataInformation)
  int i = 0;
  if (doPrints)
    {
    cerr << "SMOP::" << this << " Conditionally Temporally Update " << i << endl;
    stream 
      << vtkClientServerStream::Invoke 
      << infoHelper << "EnableStreamMessagesOn"
      << vtkClientServerStream::End;
    }
//    cerr << i << " " ;
  stream 
    << vtkClientServerStream::Invoke 
    << infoHelper << "SetInputConnection" << this->GetID()
    << vtkClientServerStream::End;
  
  stream 
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End;
  
  stream 
    << vtkClientServerStream::Invoke
    << infoHelper << "SetSplitUpdateExtent" 
    << this->PortIndex
    << vtkClientServerStream::LastResult 
    << i
    << pm->GetNumberOfPartitions(this->ConnectionID)
    << nPasses
    << 0
    << 1
    << vtkClientServerStream::End; 

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "UpdateInformation"
         << vtkClientServerStream::End;

  if (doTime)
    {
    stream 
      << vtkClientServerStream::Invoke
      << this->GetExecutiveID() << "SetUpdateTimeStep" 
      << this->PortIndex << time
      << vtkClientServerStream::End; 
    }
  
  stream 
    << vtkClientServerStream::Invoke 
    << infoHelper << "ConditionallyUpdate"
    << vtkClientServerStream::End;
  
  pm->DeleteStreamObject(infoHelper, stream);

  pm->SendPrepareProgress(this->ConnectionID);
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  pm->SendCleanupPendingProgress(this->ConnectionID);  
}

//----------------------------------------------------------------------------
void vtkSMStreamingOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


