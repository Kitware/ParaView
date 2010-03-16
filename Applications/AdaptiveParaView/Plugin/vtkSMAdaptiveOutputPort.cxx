/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveOutputPort.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveOutputPort.h"

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
#include "vtkAdaptiveOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMAdaptiveOutputPort);
vtkCxxRevisionMacro(vtkSMAdaptiveOutputPort, "1.3");

//----------------------------------------------------------------------------
vtkSMAdaptiveOutputPort::vtkSMAdaptiveOutputPort()
{

}

//----------------------------------------------------------------------------
vtkSMAdaptiveOutputPort::~vtkSMAdaptiveOutputPort()
{

}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMAdaptiveOutputPort::GetDataInformation()
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
void vtkSMAdaptiveOutputPort::InvalidateDataInformation()
{
//  cerr << this << " Invalidate Info" << endl;
  this->DataInformationValid = false;
  this->ClassNameInformationValid = false;
}


//----------------------------------------------------------------------------
// vtkPVPart used to update before gathering this information ...
void vtkSMAdaptiveOutputPort::GatherDataInformation(int vtkNotUsed(doUpdate))
{
  if (this->GetID().IsNull())
    {
    vtkErrorMacro("Part has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress(this->ConnectionID);
  this->DataInformation->Initialize();

  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke 
         << this->GetProducerID() << "UpdateInformation"
         << vtkClientServerStream::End;

  stream 
    << vtkClientServerStream::Invoke
    << this->GetExecutiveID() << "SetUpdateResolution" 
    << this->PortIndex << 0.0
    << vtkClientServerStream::End; 

  stream 
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End;

  stream
    << vtkClientServerStream::Invoke
    << this->GetExecutiveID()  << "SetSplitUpdateExtent" 
    << this->PortIndex
    << vtkClientServerStream::LastResult 
    << 0 //pass
    << pm->GetNumberOfPartitions(this->ConnectionID) //processors
    << 0 //ghostlevel
    << vtkClientServerStream::End; 

  stream 
    << vtkClientServerStream::Invoke 
    << this->GetExecutiveID() << "ComputePriority"
    << vtkClientServerStream::End;

  pm->SendStream(this->ConnectionID, this->Servers, stream);
  
  pm->GatherInformation(this->ConnectionID, this->Servers, 
                        this->DataInformation, 
                        this->GetID());
    

  this->DataInformationValid = true;

  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveOutputPort::UpdatePipelineInternal(double time, bool doTime)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

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
    << this->GetExecutiveID() << "Update"
    << vtkClientServerStream::End;

  pm->SendPrepareProgress(this->ConnectionID);
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  pm->SendCleanupPendingProgress(this->ConnectionID);  
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


