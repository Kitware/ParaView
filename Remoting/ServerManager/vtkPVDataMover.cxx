// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDataMover.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace
{

bool IsEmpty(vtkDataObject* dataObject)
{
  for (int cc = vtkDataObject::POINT; cc < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    if (cc != vtkDataObject::FIELD && dataObject->GetNumberOfElements(cc) != 0)
    {
      return false;
    }
  }
  return true;
}

std::vector<vtkSmartPointer<vtkDataObject>> CollectDataSets(
  vtkPVDataMover* self, vtkMultiProcessController* controller)
{
  auto algo = self->GetProducer();
  if (algo == nullptr || (self->GetPortNumber() >= algo->GetNumberOfOutputPorts()))
  {
    vtkLogIfF(ERROR, algo == nullptr, "missing producer!");
    vtkLogIfF(ERROR, algo != nullptr, "invalid output port number %d", self->GetPortNumber());
    return {};
  }

  const int rank = controller->GetLocalProcessId();
  const int numRanks = controller->GetNumberOfProcesses();
  const auto& sourceRanks = self->GetSourceRanks();

  auto localData = (sourceRanks.empty() ||
                     std::find(sourceRanks.begin(), sourceRanks.end(), rank) != sourceRanks.end())
    ? algo->GetOutputDataObject(self->GetPortNumber())
    : nullptr;

  if (self->GetSkipEmptyDataSets() && IsEmpty(localData))
  {
    localData = nullptr;
  }

  if (numRanks == 1)
  {
    std::vector<vtkSmartPointer<vtkDataObject>> result;
    if (localData)
    {
      result.push_back(localData);
    }
    return result;
  }

  std::vector<vtkSmartPointer<vtkDataObject>> result;
  if (self->GetGatherOnAllRanks() && vtkProcessModule::GetSymmetricMPIMode())
  {
    controller->AllGather(localData, result);
  }
  else
  {
    controller->Gather(localData, result, 0);
  }

  vtkLogIfF(TRACE, rank == 0, "collected result size = %d", (int)result.size());
  return result;
}
}

vtkStandardNewMacro(vtkPVDataMover);
vtkCxxSetObjectMacro(vtkPVDataMover, Producer, vtkAlgorithm);
//----------------------------------------------------------------------------
vtkPVDataMover::vtkPVDataMover() = default;

//----------------------------------------------------------------------------
vtkPVDataMover::~vtkPVDataMover()
{
  this->SetProducer(nullptr);
}

//----------------------------------------------------------------------------
unsigned int vtkPVDataMover::GetNumberOfDataSets() const
{
  return static_cast<unsigned int>(this->DataSets.size());
}

//----------------------------------------------------------------------------
int vtkPVDataMover::GetDataSetRank(unsigned int index) const
{
  if (index >= this->GetNumberOfDataSets())
  {
    vtkErrorMacro("Invalid index: " << index);
    return 0;
  }

  auto iter = std::next(this->DataSets.begin(), index);
  return iter->first;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVDataMover::GetDataSetAtIndex(unsigned int index) const
{
  if (index >= this->GetNumberOfDataSets())
  {
    vtkErrorMacro("Invalid index: " << index);
    return nullptr;
  }

  auto iter = std::next(this->DataSets.begin(), index);
  return iter->second;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVDataMover::GetDataSetFromRank(int rank) const
{
  auto iter = this->DataSets.find(rank);
  return iter != this->DataSets.end() ? iter->second.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
void vtkPVDataMover::AddSourceRank(int rank)
{
  this->SourceRanks.push_back(rank);
}

//----------------------------------------------------------------------------
void vtkPVDataMover::ClearAllSourceRanks()
{
  this->SourceRanks.clear();
}

//----------------------------------------------------------------------------
void vtkPVDataMover::SetSourceRanks(const std::vector<int>& ranks)
{
  this->SourceRanks = ranks;
}

//----------------------------------------------------------------------------
bool vtkPVDataMover::Execute()
{
  vtkLogScopeF(TRACE, "In Execute");
  this->DataSets.clear();

  auto pm = vtkProcessModule::GetProcessModule();
  auto session = pm ? vtkPVSession::SafeDownCast(pm->GetActiveSession()) : nullptr;
  if (!session)
  {
    vtkErrorMacro("No active session found!");
    return false;
  }

  if ((session->GetProcessRoles() & vtkPVSession::DATA_SERVER) != 0)
  {
    // process is a data-server.
    const auto vectorOfDataSets = ::CollectDataSets(this, pm->GetGlobalController());
    for (size_t cc = 0; cc < vectorOfDataSets.size(); ++cc)
    {
      if (auto dObj = vectorOfDataSets[cc])
      {
        this->DataSets[static_cast<int>(cc)] = dObj;
      }
    }
    vtkLogF(TRACE, "data-server-size: %d", (int)this->DataSets.size());
  }

  // get the controller to talk between client & server, if any.
  if (auto sController = session->GetController(vtkPVSession::DATA_SERVER_ROOT))
  {
    int numDataSets;
    sController->Receive(&numDataSets, 1, 1, 78111);
    if (numDataSets > 0)
    {
      std::vector<int> ranks(numDataSets);
      sController->Receive(&ranks[0], numDataSets, 1, 78112);
      for (int cc = 0; cc < numDataSets; ++cc)
      {
        auto dobj = vtk::TakeSmartPointer(sController->ReceiveDataObject(1, 78113));
        this->DataSets[ranks[cc]] = std::move(dobj);
      }
    }
  }
  else if (auto cController = session->GetController(vtkPVSession::CLIENT))
  {
    const int numDataSets = static_cast<int>(this->DataSets.size());
    cController->Send(&numDataSets, 1, 1, 78111);
    if (numDataSets > 0)
    {
      std::vector<int> ranks(numDataSets);
      std::transform(this->DataSets.begin(), this->DataSets.end(), ranks.begin(),
        [](const std::pair<int, vtkSmartPointer<vtkDataObject>>& pair) { return pair.first; });
      cController->Send(&ranks[0], numDataSets, 1, 78112);
      for (const auto& pair : this->DataSets)
      {
        cController->Send(pair.second, 1, 78113);
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkPVDataMover::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Producer: " << this->Producer << endl;
  os << indent << "PortNumber: " << this->PortNumber << endl;
  os << indent << "GatherOnAllRanks: " << this->GatherOnAllRanks << endl;
  os << indent << "SourceRanks: (count=" << this->SourceRanks.size() << ")" << endl;
  for (const auto& rank : this->SourceRanks)
  {
    os << indent.GetNextIndent() << rank << endl;
  }
}
