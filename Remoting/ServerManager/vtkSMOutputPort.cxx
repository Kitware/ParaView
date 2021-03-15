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
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMOutputPort);

//----------------------------------------------------------------------------
vtkSMOutputPort::vtkSMOutputPort()
{
  this->ClassNameInformation = vtkPVClassNameInformation::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->TemporalDataInformation = vtkPVTemporalDataInformation::New();
  this->ClassNameInformationValid = 0;
  this->DataInformationValid = false;
  this->TemporalDataInformationValid = false;
  this->PortIndex = 0;
  this->SourceProxy = nullptr;
  this->CompoundSourceProxy = nullptr;
  this->ObjectsCreated = 1;
}

//----------------------------------------------------------------------------
vtkSMOutputPort::~vtkSMOutputPort()
{
  this->SetSourceProxy(nullptr);
  this->ClassNameInformation->Delete();
  this->DataInformation->Delete();
  this->TemporalDataInformation->Delete();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataInformation()
{
  if (!this->DataInformationValid)
  {
    std::ostringstream mystr;
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
vtkPVDataInformation* vtkSMOutputPort::GetSubsetDataInformation(
  const char* selector, const char* assemblyName)
{
  auto dinfo = this->GetDataInformation();
  auto assembly = dinfo->GetDataAssembly(assemblyName);
  if (assembly == nullptr || selector == nullptr || selector[0] == '\0')
  {
    return nullptr;
  }

  const auto nodes = assembly->SelectNodes({ selector });
  if (nodes.size() == 0)
  {
    return nullptr;
  }
  if (nodes.size() > 1)
  {
    vtkWarningMacro(
      "GetSubsetDataInformation selector matched multiple nodes. Only first one is used.");
  }

  const std::string key(assemblyName ? assemblyName : "");

  auto iter1 = this->SubsetDataInformations.find(key);
  if (iter1 != this->SubsetDataInformations.end())
  {
    auto iter2 = iter1->second.find(nodes.front());
    if (iter2 != iter1->second.end())
    {
      return iter2->second;
    }
  }

  this->SourceProxy->GetSession()->PrepareProgress();

  vtkNew<vtkPVDataInformation> subsetInfo;
  subsetInfo->Initialize();
  subsetInfo->SetPortNumber(this->PortIndex);
  subsetInfo->SetSubsetSelector(selector);
  subsetInfo->SetSubsetAssemblyName(assemblyName);
  this->SourceProxy->GatherInformation(subsetInfo);

  this->SubsetDataInformations[key][nodes.front()] = subsetInfo;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
  return subsetInfo;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetSubsetDataInformation(unsigned int compositeIndex)
{
  auto dinfo = this->GetDataInformation();
  if (dinfo->DataSetTypeIsA(VTK_MULTIBLOCK_DATA_SET))
  {
    auto hierarchy = dinfo->GetHierarchy();
    return this->GetSubsetDataInformation(
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(compositeIndex, hierarchy).c_str(),
      vtkDataAssemblyUtilities::HierarchyName());
  }

  vtkWarningMacro("GetSelectorForCompositeId(compositeIndex) called for a non-multiblock dataset.");
  return nullptr;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation* vtkSMOutputPort::GetClassNameInformation()
{
  if (this->ClassNameInformationValid == 0)
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
  this->SubsetDataInformations.clear();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherDataInformation()
{
  if (!this->SourceProxy)
  {
    vtkErrorMacro("Invalid vtkSMOutputPort.");
    return;
  }

  this->SourceProxy->GetSession()->PrepareProgress();
  this->DataInformation->Initialize();
  this->DataInformation->SetPortNumber(this->PortIndex);
  this->SourceProxy->GatherInformation(this->DataInformation);
  this->DataInformation->Modified();

  this->DataInformationValid = true;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherTemporalDataInformation()
{
  if (!this->SourceProxy)
  {
    vtkErrorMacro("Invalid vtkSMOutputPort.");
    return;
  }

  this->SourceProxy->GetSession()->PrepareProgress();
  this->TemporalDataInformation->Initialize();
  this->TemporalDataInformation->SetPortNumber(this->PortIndex);
  this->SourceProxy->GatherInformation(this->TemporalDataInformation);
  this->TemporalDataInformation->Modified();

  this->TemporalDataInformationValid = true;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherClassNameInformation()
{
  if (!this->SourceProxy)
  {
    vtkErrorMacro("Invalid vtkSMOutputPort.");
    return;
  }

  this->ClassNameInformation->SetPortNumber(this->PortIndex);
  vtkObjectBase* cso = this->SourceProxy->GetClientSideObject();
  if (cso)
  {
    this->ClassNameInformation->CopyFromObject(
      vtkAlgorithm::SafeDownCast(cso)->GetOutputDataObject(this->PortIndex));
  }
  else
  {
    this->SourceProxy->GatherInformation(this->ClassNameInformation);
  }
  this->ClassNameInformationValid = 1;
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
void vtkSMOutputPort::UpdatePipelineInternal(double time, bool doTime)
{
  this->SourceProxy->GetSession()->PrepareProgress();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIPROXY(this->SourceProxy) << "UpdatePipeline"
         << this->PortIndex << time << (doTime ? 1 : 0) << vtkClientServerStream::End;
  this->SourceProxy->ExecuteStream(stream);
  this->SourceProxy->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PortIndex: " << this->PortIndex << endl;
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMOutputPort::GetSourceProxy()
{
  return this->CompoundSourceProxy ? this->CompoundSourceProxy.GetPointer()
                                   : this->SourceProxy.GetPointer();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetSourceProxy(vtkSMSourceProxy* src)
{
  this->SourceProxy = src;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetCompoundSourceProxy(vtkSMCompoundSourceProxy* src)
{
  this->CompoundSourceProxy = src;
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMOutputPort::GetSession()
{
  return this->SourceProxy ? this->SourceProxy->GetSession() : nullptr;
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMOutputPort::GetSessionProxyManager()
{
  return this->SourceProxy ? this->SourceProxy->GetSessionProxyManager() : nullptr;
}
