// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMOutputPort.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMOutputPort);

//----------------------------------------------------------------------------
vtkSMOutputPort::vtkSMOutputPort()
{
  this->ObjectsCreated = 1;
}

//----------------------------------------------------------------------------
vtkSMOutputPort::~vtkSMOutputPort() = default;

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataInformation()
{
  if (!this->DataInformationValid)
  {
    std::ostringstream mystr;
    mystr << this->GetSourceProxy()->GetXMLName() << "::GatherInformation";
    vtkTimerLog::MarkStartEvent(mystr.str().c_str());
    this->DataInformation->SetPortNumber(this->PortIndex);
    this->GatherInformation(this->DataInformation);
    this->DataInformationValid = true;
    vtkTimerLog::MarkEndEvent(mystr.str().c_str());
  }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataSetInformation()
{
  if (!this->DataSetInformationValid)
  {
    std::ostringstream mystr;
    mystr << this->GetSourceProxy()->GetXMLName() << "::GatherSetInformation";
    vtkTimerLog::MarkStartEvent(mystr.str().c_str());
    this->DataInformation->SetInspectCells(true);
    this->DataInformation->SetPortNumber(this->PortIndex);
    this->GatherInformation(this->DataInformation);
    this->DataSetInformationValid = true;
    vtkTimerLog::MarkEndEvent(mystr.str().c_str());
  }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation* vtkSMOutputPort::GetTemporalDataInformation()
{
  if (!this->TemporalDataInformationValid)
  {
    this->TemporalDataInformation->SetPortNumber(this->PortIndex);
    this->GatherInformation(this->TemporalDataInformation);
    this->TemporalDataInformationValid = true;
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
  if (nodes.empty())
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

  vtkNew<vtkPVDataInformation> subsetInfo;
  subsetInfo->SetPortNumber(this->PortIndex);
  subsetInfo->SetSubsetSelector(selector);
  subsetInfo->SetSubsetAssemblyName(assemblyName);
  this->GatherInformation(subsetInfo);

  this->SubsetDataInformations[key][nodes.front()] = subsetInfo;
  return subsetInfo;
}

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation* vtkSMOutputPort::GetTemporalSubsetDataInformation(
  const char* selector, const char* assemblyName)
{
  auto dinfo = this->GetTemporalDataInformation();
  auto assembly = dinfo->GetDataAssembly(assemblyName);
  if (assembly == nullptr || selector == nullptr || selector[0] == '\0')
  {
    return nullptr;
  }

  const auto nodes = assembly->SelectNodes({ selector });
  if (nodes.empty())
  {
    return nullptr;
  }
  if (nodes.size() > 1)
  {
    vtkWarningMacro(
      "GetTemporalSubsetDataInformation selector matched multiple nodes. Only first one is used.");
  }

  const std::string key(assemblyName ? assemblyName : "");

  auto iter1 = this->TemporalSubsetDataInformations.find(key);
  if (iter1 != this->TemporalSubsetDataInformations.end())
  {
    auto iter2 = iter1->second.find(nodes.front());
    if (iter2 != iter1->second.end())
    {
      return iter2->second;
    }
  }

  vtkNew<vtkPVTemporalDataInformation> temporalSubsetInfo;
  temporalSubsetInfo->SetPortNumber(this->PortIndex);
  temporalSubsetInfo->SetSubsetSelector(selector);
  temporalSubsetInfo->SetSubsetAssemblyName(assemblyName);

  this->GatherInformation(temporalSubsetInfo);

  this->TemporalSubsetDataInformations[key][nodes.front()] = temporalSubsetInfo;

  return temporalSubsetInfo;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetSubsetDataInformation(unsigned int compositeIndex)
{
  auto dinfo = this->GetDataInformation();
  if (dinfo->DataSetTypeIsA(VTK_MULTIBLOCK_DATA_SET))
  {
    if (compositeIndex == 0)
    {
      // Bug #20997 return the full data information for composite index 0
      return dinfo;
    }
    auto hierarchy = dinfo->GetHierarchy();
    return this->GetSubsetDataInformation(
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(compositeIndex, hierarchy).c_str(),
      vtkDataAssemblyUtilities::HierarchyName());
  }

  vtkWarningMacro("GetSelectorForCompositeId(compositeIndex) called for a non-multiblock dataset.");
  return nullptr;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetRankDataInformation(int rank)
{
  auto session = this->GetSession();
  const auto numRanks = session->GetNumberOfProcesses(this->GetSourceProxy()->GetLocation());
  if (rank < 0 || (numRanks == 1 && rank == 0))
  {
    // same as regular data information.
    return this->GetDataInformation();
  }

  if (rank >= numRanks)
  {
    // raise an error, since this may not be what the developer expected.
    vtkErrorMacro("Incorrect rank requested!");
    return this->GetDataInformation();
  }

  auto iter = this->RankDataInformations.find(rank);
  if (iter != this->RankDataInformations.end())
  {
    return iter->second;
  }

  this->SourceProxy->GetSession()->PrepareProgress();

  vtkNew<vtkPVDataInformation> rankInfo;
  rankInfo->Initialize();
  rankInfo->SetPortNumber(this->PortIndex);
  rankInfo->SetRank(rank);
  this->SourceProxy->GatherInformation(rankInfo);
  this->RankDataInformations[rank] = rankInfo;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
  return rankInfo;
}

//----------------------------------------------------------------------------
vtkPVClassNameInformation* vtkSMOutputPort::GetClassNameInformation()
{
  if (!this->ClassNameInformationValid)
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
  this->RankDataInformations.clear();
  this->TemporalSubsetDataInformations.clear();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherInformation(vtkPVDataInformation* info)
{
  if (!this->SourceProxy)
  {
    vtkErrorMacro("Invalid vtkSMOutputPort.");
    return;
  }

  this->SourceProxy->GetSession()->PrepareProgress();
  info->Initialize();
  this->SourceProxy->GatherInformation(info);
  info->Modified();

  this->SourceProxy->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherTemporalDataInformation()
{
  this->TemporalDataInformation->SetPortNumber(this->PortIndex);
  this->GatherInformation(this->TemporalDataInformation);
  this->TemporalDataInformationValid = true;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::GatherDataInformation()
{
  this->DataInformation->SetPortNumber(this->PortIndex);
  this->GatherInformation(this->DataInformation);
  this->DataInformationValid = true;
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
  this->ClassNameInformationValid = true;
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
