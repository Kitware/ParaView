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
#include "vtkSMPartDisplay.h"
#include "vtkSMSummaryHelperProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProperty.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMXMLPVAnimationWriterProxy);
vtkCxxRevisionMacro(vtkSMXMLPVAnimationWriterProxy, "1.1");
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
    pm->SendStream(this->Servers, stream);
    }
  delete this->Internals;
  if (this->SummaryHelperProxy)
    {
    this->SummaryHelperProxy->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->SetServers(vtkProcessModule::DATA_SERVER);
  this->Superclass::CreateVTKObjects(numObjects);
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions();
  
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke
      << this->GetID(cc) << "SetNumberOfPieces" <<
      numPartitions << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->GetID(cc) << "SetPiece" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, stream);
    }
}
//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::AddInput(vtkSMProxy *input)
{
  vtkSMPartDisplay* pdp = vtkSMPartDisplay::SafeDownCast(input);
  if (!pdp)
    {
    vtkErrorMacro("Input can only be a vtkSMPartDisplayProxy or derrived classes.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions();
  vtkClientServerStream stream;
  
  if (numPartitions > 1)
    {
    // Since we don't have the PVSource name. To mimic that 
    // we will just create unique names  here.
    static int name_count = 0;
    ostrstream str;
    str << "source" << name_count++ << ends;
    
    vtkClientServerID id = pm->NewStreamObject("vtkCompleteArrays", stream);
    pdp->ConnectGeometryForWriting(id, "SetInput", &stream);
    this->Internals->IDs.push_back(id);

    for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
      {
      stream << vtkClientServerStream::Invoke
        << id << "GetOutput" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
        << this->GetID(cc) << "AddInput" << vtkClientServerStream::LastResult
        << str.str() << vtkClientServerStream::End;
      }
    str.rdbuf()->freeze(0);
    }
  else
    {
    for(unsigned int cc=0; cc< this->GetNumberOfIDs(); cc++)
      {
      pdp->ConnectGeometryForWriting(this->GetID(cc), "AddInput", &stream);
      }
    }
  pm->SendStream(this->Servers, stream);
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
  
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc) 
      << "WriteTime" << time << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
      << "GetErrorCode" << vtkClientServerStream::End;
    }
  
  pm->SendStream(this->Servers, stream);
  int retVal =0;
  pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(
    0,0, &retVal);
  this->ErrorCode = retVal;
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::Start()
{
  this->ErrorCode = 0;
  vtkClientServerStream str;
  
  // Check if SummaryHelperProxy is needed.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int numPartitions = pm->GetNumberOfPartitions();
  if (numPartitions > 1)
    {
    if (!this->SummaryHelperProxy)
      {
      vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      this->SummaryHelperProxy = vtkSMSummaryHelperProxy::SafeDownCast(
        pxm->NewProxy("writers","SummaryHelper"));
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
  
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc) << "Start" << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str);
    }
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::Finish()
{
  vtkClientServerStream str;
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    str << vtkClientServerStream::Invoke
      << this->GetID(cc) << "Finish" << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
      << this->GetID(cc) << "GetErrorCode" << vtkClientServerStream::End;
    }
  int retVal = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, str);
  pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal);
  this->ErrorCode = retVal;
}

//-----------------------------------------------------------------------------
void vtkSMXMLPVAnimationWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ErrorCode: " << this->ErrorCode << endl;
  os << indent << "SummaryHelperProxy: " << this->SummaryHelperProxy << endl;

}
