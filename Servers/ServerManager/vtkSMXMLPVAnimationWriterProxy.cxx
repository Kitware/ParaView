/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXMLPVAnimationWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXMLPVAnimationWriterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMSummaryHelperProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProperty.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMXMLPVAnimationWriterProxy);
vtkCxxRevisionMacro(vtkSMXMLPVAnimationWriterProxy, "1.6");
//*****************************************************************************
class vtkSMXMLPVAnimationWriterProxyInternals
{
public:
  typedef vtkstd::vector<vtkClientServerID> ClientServerIDVector;
  ClientServerIDVector IDs;
};

//*****************************************************************************

//-----------------------------------------------------------------------------
vtkSMXMLPVAnimationWriterProxy::vtkSMXMLPVAnimationWriterProxy()
{
  this->SetServers(vtkProcessModule::DATA_SERVER);
  this->Internals = new vtkSMXMLPVAnimationWriterProxyInternals;
  this->SummaryHelperProxy = NULL;
  this->ErrorCode = 0;
  this->SetExecutiveName(0);
}

//-----------------------------------------------------------------------------
vtkSMXMLPVAnimationWriterProxy::~vtkSMXMLPVAnimationWriterProxy()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkSMXMLPVAnimationWriterProxyInternals::ClientServerIDVector::iterator i
    = this->Internals->IDs.begin();
  for (; i != this->Internals->IDs.end(); i++)
    {
    pm->DeleteStreamObject(*i, stream);
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  delete this->Internals;
  if (this->SummaryHelperProxy)
    {
    this->SummaryHelperProxy->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
    
  this->SetServers(vtkProcessModule::DATA_SERVER);
  this->Superclass::CreateVTKObjects();
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions(this->ConnectionID);
  
  stream << vtkClientServerStream::Invoke
         << this->GetID() << "SetNumberOfPieces" <<
    numPartitions << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID() << "SetPiece" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}
//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::AddInput(vtkSMSourceProxy *input,
                                              const char* method, int)
{

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions(this->ConnectionID);
  vtkClientServerStream stream;
 
  this->CreateVTKObjects();

  // Since we don't have the PVSource name. To mimic that 
  // we will just create unique names  here.
  static int name_count = 0;
  ostrstream groupname_str;
  groupname_str << "source" << name_count++ << ends;

  // when numPartitions > 1, for the vtkXMLPVAnimationWriter to treat the
  // different parts as multiple parts of the same input, we
  // have to specify the same group name.
  if (numPartitions > 1)
    {
    vtkClientServerID ca_id = pm->NewStreamObject("vtkCompleteArrays", stream);
    this->Internals->IDs.push_back(ca_id);

    stream << vtkClientServerStream::Invoke
           << input->GetID() << "GetOutput" 
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << ca_id << "SetInput" 
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    
    stream << vtkClientServerStream::Invoke
           << ca_id << "GetOutput" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->GetID() << method << vtkClientServerStream::LastResult
           << groupname_str.str() << vtkClientServerStream::End;
    }
  else
    {
    stream << vtkClientServerStream::Invoke
           << input->GetID() << "GetOutput" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->GetID() << method << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  groupname_str.rdbuf()->freeze(0);
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::WriteTime(double time)
{
  if (this->ErrorCode)
    {
    vtkErrorMacro("Error has been detected. Writing aborted.");
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  stream << vtkClientServerStream::Invoke << this->GetID() 
         << "WriteTime" << time << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->GetID()
         << "GetErrorCode" << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  int retVal =0;
  pm->GetLastResult(this->ConnectionID, 
    vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0,0, &retVal);
  this->ErrorCode = retVal;
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::Start()
{
  this->ErrorCode = 0;
  vtkClientServerStream str;
  
  // Check if SummaryHelperProxy is needed.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions(this->ConnectionID);
  if (numPartitions > 1)
    {
    if (!this->SummaryHelperProxy)
      {
      vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      this->SummaryHelperProxy = vtkSMSummaryHelperProxy::SafeDownCast(
        pxm->NewProxy("writers","SummaryHelper"));
      if (this->SummaryHelperProxy)
        {
        this->SummaryHelperProxy->SetConnectionID(this->ConnectionID);
        }
      }
    if (!this->SummaryHelperProxy)
      {
      vtkErrorMacro("Failed to create SummaryHelperProxy");
      return;
      }
    
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->SummaryHelperProxy->GetProperty("Writer"));
    pp->RemoveAllProxies();
    pp->AddProxy(this);
    this->SummaryHelperProxy->UpdateVTKObjects();

    vtkSMProperty* p = this->SummaryHelperProxy->GetProperty("SynchronizeSummaryFiles");
    p->Modified();
    this->SummaryHelperProxy->UpdateVTKObjects();
    }
  
  str << vtkClientServerStream::Invoke
      << this->GetID() << "Start" << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::Finish()
{
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID() << "Finish" << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID() << "GetErrorCode" << vtkClientServerStream::End;

  int retVal = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->ConnectionID, this->Servers, str);
  pm->GetLastResult(this->ConnectionID,
    vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal);
  this->ErrorCode = retVal;
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ErrorCode: " << this->ErrorCode << endl;
  os << indent << "SummaryHelperProxy: " << this->SummaryHelperProxy << endl;

}
