/*=========================================================================

  Program:   ParaView
  Module:    vtkSpreadSheetView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpreadSheetView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCSVExporter.h"
#include "vtkCharArray.h"
#include "vtkClientServerMoveData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkMarkSelectedRows.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVMergeTables.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkSortedTableStreamer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkSpreadSheetRepresentation.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
namespace
{
struct OrderByNames : std::binary_function<vtkAbstractArray*, vtkAbstractArray*, bool>
{
  bool operator()(vtkAbstractArray* a1, vtkAbstractArray* a2)
  {
    const char* order[] = { "vtkOriginalProcessIds", "vtkCompositeIndexArray", "vtkOriginalIndices",
      "vtkOriginalCellIds", "vtkOriginalPointIds", "vtkOriginalRowIds", "Structured Coordinates",
      NULL };
    std::string a1Name = a1->GetName() ? a1->GetName() : "";
    std::string a2Name = a2->GetName() ? a2->GetName() : "";
    int a1Index = VTK_INT_MAX, a2Index = VTK_INT_MAX;
    for (int cc = 0; order[cc] != NULL; cc++)
    {
      if (a1Index == VTK_INT_MAX && a1Name == order[cc])
      {
        a1Index = cc;
      }
      if (a2Index == VTK_INT_MAX && a2Name == order[cc])
      {
        a2Index = cc;
      }
    }
    if (a1Index < a2Index)
    {
      return true;
    }
    if (a2Index < a1Index)
    {
      return false;
    }
    // we can reach here only when both array names are not in the "priority"
    // set or they are the same (which does happen, see BUG #9808).
    assert((a1Index == VTK_INT_MAX && a2Index == VTK_INT_MAX) || (a1Name == a2Name));
    return (a1Name < a2Name);
  }
};

/// internal function to convert any array's name to a user friendly name.
const char* get_userfriendly_name(
  const char* name, vtkSpreadSheetView* self, bool* converted = nullptr)
{
  if (converted)
  {
    *converted = true;
  }
  if (name == nullptr || name[0] == '\0')
  {
    if (converted)
    {
      *converted = false;
    }
    return name;
  }
  else if (strcmp("vtkOriginalProcessIds", name) == 0)
  {
    return "Process ID";
  }
  else if (strcmp("vtkOriginalIndices", name) == 0)
  {
    switch (self->GetFieldAssociation())
    {
      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        return "Point ID";
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        return "Cell ID";
      case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
        return "Vertex ID";
      case vtkDataObject::FIELD_ASSOCIATION_EDGES:
        return "Edge ID";
      case vtkDataObject::FIELD_ASSOCIATION_ROWS:
        return "Row ID";
      default:
        // LOG_S(INFO) << "Unknown field association encountered.";
        return name;
    }
  }
  else if (strcmp("vtkOriginalCellIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Cell ID";
  }
  else if (strcmp("vtkOriginalPointIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Point ID";
  }
  else if (strcmp("vtkOriginalRowIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Row ID";
  }
  else if (strcmp("vtkCompositeIndexArray", name) == 0)
  {
    return "Block Number";
  }

  if (converted)
  {
    *converted = false;
  }
  return name;
}
}

class vtkSpreadSheetView::vtkInternals
{
  std::vector<std::tuple<std::string, std::string, int> > ColumnMetaData;
  std::map<std::string, size_t> ColumnIndexMap;

  void UpdateColumnMetaData(vtkTable* table)
  {
    this->ColumnMetaData.clear();
    this->ColumnIndexMap.clear();

    std::map<std::string, vtkIdType> index_map; // this is just to make the lookup faster.
    for (vtkIdType cc = 0, max = table->GetNumberOfColumns(); cc < max; ++cc)
    {
      // this build a tuple that indicates it's not an extracted component
      auto col = table->GetColumn(cc);
      auto colInfo = col->GetInformation();

      const std::string original_name =
        colInfo->Has(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME())
        ? std::string(colInfo->Get(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME()))
        : std::string();

      const int original_component =
        colInfo->Has(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER())
        ? colInfo->Get(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER())
        : -1;

      std::string strName = "<None>";
      const char* colName = col->GetName();
      if (colName)
      {
        strName = std::string(colName);
      }

      auto tuple = std::make_tuple(
        strName, original_component >= 0 ? original_name : std::string(), original_component);
      this->ColumnIndexMap[std::get<0>(tuple)] = this->ColumnMetaData.size();
      this->ColumnMetaData.push_back(std::move(tuple));
    }

    assert(this->ColumnMetaData.size() == this->ColumnIndexMap.size() &&
      this->ColumnIndexMap.size() == static_cast<size_t>(table->GetNumberOfColumns()));
  }

  vtkIdType GetMostRecentlyAccessedBlock(vtkSpreadSheetView* self)
  {
    vtkIdType maxBlockId = self->GetNumberOfRows() / self->TableStreamer->GetBlockSize();
    if (this->MostRecentlyAccessedBlock >= 0 && this->MostRecentlyAccessedBlock <= maxBlockId)
    {
      return this->MostRecentlyAccessedBlock;
    }
    this->MostRecentlyAccessedBlock = 0;
    return 0;
  }

  class CacheInfo
  {
  public:
    vtkSmartPointer<vtkTable> Dataobject;
    vtkTimeStamp RecentUseTime;
  };

  typedef std::map<vtkIdType, CacheInfo> CacheType;
  CacheType CachedBlocks;

public:
  void ClearCache()
  {
    this->CachedBlocks.clear();
    this->ColumnMetaData.clear();
    this->ColumnIndexMap.clear();
  }

  vtkIdType GetNumberOfColumns(vtkSpreadSheetView* self)
  {
    if (this->ActiveRepresentation != nullptr && this->ColumnMetaData.size() == 0)
    {
      this->GetSomeBlock(self);
    }
    return static_cast<vtkIdType>(this->ColumnMetaData.size());
  }

  const char* GetColumnName(vtkIdType index, vtkSpreadSheetView* self)
  {
    if (this->GetNumberOfColumns(self) > index)
    {
      return std::get<0>(this->ColumnMetaData[index]).c_str();
    }
    return nullptr;
  }

  std::string GetOriginalArrayName(const std::string& aname) const
  {
    auto iter = this->ColumnIndexMap.find(aname);
    if (iter != this->ColumnIndexMap.end())
    {
      const auto& tuple = this->ColumnMetaData[iter->second];
      return !std::get<1>(tuple).empty() ? std::get<1>(tuple) : aname;
    }
    return aname;
  }

  vtkTable* GetDataObject(vtkIdType blockId)
  {
    CacheType::iterator iter = this->CachedBlocks.find(blockId);
    if (iter != this->CachedBlocks.end())
    {
      iter->second.RecentUseTime.Modified();
      this->MostRecentlyAccessedBlock = blockId;
      return iter->second.Dataobject.GetPointer();
    }
    return NULL;
  }

  void AddToCache(vtkIdType blockId, vtkTable* data, vtkIdType max)
  {
    CacheType::iterator iter = this->CachedBlocks.find(blockId);
    if (iter != this->CachedBlocks.end())
    {
      this->CachedBlocks.erase(iter);
    }

    if (static_cast<vtkIdType>(this->CachedBlocks.size()) == max)
    {
      // remove least-recent-used block.
      iter = this->CachedBlocks.begin();
      CacheType::iterator iterToRemove = this->CachedBlocks.begin();
      for (; iter != this->CachedBlocks.end(); ++iter)
      {
        if (iterToRemove->second.RecentUseTime > iter->second.RecentUseTime)
        {
          iterToRemove = iter;
        }
      }
      this->CachedBlocks.erase(iterToRemove);
    }

    CacheInfo info;
    vtkTable* clone = vtkTable::New();

    // sort columns for better usability.
    std::vector<vtkAbstractArray*> arrays;
    for (vtkIdType cc = 0; cc < data->GetNumberOfColumns(); cc++)
    {
      if (data->GetColumn(cc))
      {
        arrays.push_back(data->GetColumn(cc));
      }
    }
    std::sort(arrays.begin(), arrays.end(), OrderByNames());
    for (std::vector<vtkAbstractArray*>::iterator viter = arrays.begin(); viter != arrays.end();
         ++viter)
    {
      clone->AddColumn(*viter);
    }
    info.Dataobject = clone;
    clone->FastDelete();
    info.RecentUseTime.Modified();
    this->CachedBlocks[blockId] = info;
    this->MostRecentlyAccessedBlock = blockId;

    if (this->CachedBlocks.size() == 1)
    {
      this->UpdateColumnMetaData(clone);
    }
  }

  /**
   * A convenient method to get some block. It returns either a cached block
   * or the most recently accessed block, if possible. This method will avoid a
   * fetch unless needed.
   */
  vtkTable* GetSomeBlock(vtkSpreadSheetView* self)
  {
    const auto mrbId = this->GetMostRecentlyAccessedBlock(self);
    if (auto table = this->GetDataObject(mrbId))
    {
      return table;
    }

    for (const auto& cinfo : this->CachedBlocks)
    {
      if (cinfo.second.Dataobject != nullptr)
      {
        return cinfo.second.Dataobject;
      }
    }
    return self->FetchBlock(mrbId);
  }

  vtkIdType MostRecentlyAccessedBlock;
  vtkWeakPointer<vtkSpreadSheetRepresentation> ActiveRepresentation;
  vtkCommand* Observer;

  std::set<std::string> HiddenColumnsByName;
  std::set<std::string> HiddenColumnsByLabel;
};

namespace
{
void FetchRMI(void* localArg, void* remoteArg, int remoteArgLength, int)
{
  assert(remoteArgLength == sizeof(vtkTypeUInt64) * 2);
  (void)remoteArgLength;

  auto arg = reinterpret_cast<vtkTypeUInt64*>(remoteArg);
  vtkSpreadSheetView* self = reinterpret_cast<vtkSpreadSheetView*>(localArg);
  if (static_cast<vtkTypeUInt32>(self->GetIdentifier()) == arg[0])
  {
    self->FetchBlockCallback(static_cast<vtkIdType>(arg[1]));
  }
}

unsigned long vtkCountNumberOfRows(vtkDataObject* dobj)
{
  vtkTable* table = vtkTable::SafeDownCast(dobj);
  if (table)
  {
    return table->GetNumberOfRows();
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cd)
  {
    unsigned long count = 0;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      count += vtkCountNumberOfRows(iter->GetCurrentDataObject());
    }
    iter->Delete();
    return count;
  }
  return 0;
}

vtkAlgorithmOutput* vtkGetDataProducer(vtkSpreadSheetView* self, vtkSpreadSheetRepresentation* repr)
{
  if (repr)
  {
    if (self->GetShowExtractedSelection())
    {
      return repr->GetExtractedDataProducer();
    }
    else
    {
      return repr->GetDataProducer();
    }
  }
  return NULL;
}

#if 0 // Its usage is commented out below.
  vtkAlgorithmOutput* vtkGetSelectionProducer(
    vtkSpreadSheetView* self, vtkSpreadSheetRepresentation* repr)
    {
    if (repr)
      {
      if (self->GetShowExtractedSelection())
        {
        return NULL;
        }
      return repr->GetSelectionProducer();
      }
    return NULL;
    }
#endif
}

vtkStandardNewMacro(vtkSpreadSheetView);
//----------------------------------------------------------------------------
vtkSpreadSheetView::vtkSpreadSheetView()
  : Superclass(/*create_render_window=*/false)
  , CRMICallbackTag(0)
  , PRMICallbackTag(0)
  , Identifier(0)
{
  this->NumberOfRows = 0;
  this->ShowExtractedSelection = false;
  this->TableStreamer = vtkSortedTableStreamer::New();
  this->TableSelectionMarker = vtkMarkSelectedRows::New();

  this->ReductionFilter = vtkReductionFilter::New();
  this->ReductionFilter->SetController(vtkMultiProcessController::GetGlobalController());

  vtkPVMergeTables* post_gather_algo = vtkPVMergeTables::New();
  this->ReductionFilter->SetPostGatherHelper(post_gather_algo);
  post_gather_algo->FastDelete();

  this->DeliveryFilter = vtkClientServerMoveData::New();
  this->DeliveryFilter->SetOutputDataType(VTK_TABLE);

  this->ReductionFilter->SetInputConnection(this->TableStreamer->GetOutputPort());

  this->Internals = new vtkInternals();
  this->Internals->MostRecentlyAccessedBlock = -1;

  this->Internals->Observer =
    vtkMakeMemberFunctionCommand(*this, &vtkSpreadSheetView::OnRepresentationUpdated);
  this->SomethingUpdated = false;

  auto session = this->GetSession();
  assert(session);
  if (auto cController = session->GetController(vtkPVSession::CLIENT))
  {
    this->CRMICallbackTag = cController->AddRMICallback(::FetchRMI, this, FETCH_BLOCK_TAG);
  }
  if (auto pController = vtkMultiProcessController::GetGlobalController())
  {
    this->PRMICallbackTag = pController->AddRMICallback(::FetchRMI, this, FETCH_BLOCK_TAG);
  }
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
}

//----------------------------------------------------------------------------
vtkSpreadSheetView::~vtkSpreadSheetView()
{
  auto session = this->GetSession();
  if (auto cController = session ? session->GetController(vtkPVSession::CLIENT) : nullptr)
  {
    cController->RemoveRMICallback(this->CRMICallbackTag);
    this->CRMICallbackTag = 0;
  }
  if (auto pController = session ? vtkMultiProcessController::GetGlobalController() : nullptr)
  {
    pController->RemoveRMICallback(this->PRMICallbackTag);
    this->PRMICallbackTag = 0;
  }

  this->TableStreamer->Delete();
  this->TableSelectionMarker->Delete();
  this->ReductionFilter->Delete();
  this->DeliveryFilter->Delete();

  this->Internals->Observer->Delete();
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetShowExtractedSelection(bool val)
{
  if (val != this->ShowExtractedSelection)
  {
    this->ShowExtractedSelection = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::HideColumnByName(const char* columnName)
{
  if (columnName)
  {
    auto& internals = *this->Internals;
    internals.HiddenColumnsByName.insert(columnName);
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearHiddenColumnsByName()
{
  auto& internals = *this->Internals;
  internals.HiddenColumnsByName.clear();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnHiddenByName(const char* columnName)
{
  const auto& internals = *this->Internals;
  return columnName
    ? internals.HiddenColumnsByName.find(columnName) != internals.HiddenColumnsByName.end()
    : true;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::HideColumnByLabel(const char* columnLabel)
{
  if (columnLabel)
  {
    auto& internals = *this->Internals;
    internals.HiddenColumnsByLabel.insert(columnLabel);
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearHiddenColumnsByLabel()
{
  auto& internals = *this->Internals;
  internals.HiddenColumnsByLabel.clear();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnHiddenByLabel(const std::string& columnLabel)
{
  const auto& internals = *this->Internals;
  return columnLabel.empty() ? true : internals.HiddenColumnsByLabel.find(columnLabel) !=
      internals.HiddenColumnsByLabel.end();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearCache()
{
  this->Internals->ClearCache();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::GetColumnVisibility(vtkIdType index)
{
  auto name = this->GetColumnName(index);
  auto label = this->GetColumnLabel(name);
  return !(this->IsColumnInternal(name) || this->IsColumnHiddenByName(name) ||
    this->IsColumnHiddenByLabel(label));
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnInternal(vtkIdType index)
{
  return this->IsColumnInternal(this->GetColumnName(index));
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnInternal(const char* columnName)
{
  return (columnName == nullptr || strcmp(columnName, "__vtkIsSelected__") == 0) ? true : false;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::Update()
{
  vtkSpreadSheetRepresentation* prev = this->Internals->ActiveRepresentation;
  vtkSpreadSheetRepresentation* cur = NULL;
  for (int cc = 0; cc < this->GetNumberOfRepresentations(); cc++)
  {
    vtkSpreadSheetRepresentation* repr =
      vtkSpreadSheetRepresentation::SafeDownCast(this->GetRepresentation(cc));
    if (repr && repr->GetVisibility())
    {
      cur = repr;
      break;
    }
  }

  if (prev != cur)
  {
    if (prev)
    {
      prev->RemoveObserver(this->Internals->Observer);
    }
    if (cur)
    {
      cur->AddObserver(vtkCommand::UpdateDataEvent, this->Internals->Observer);
    }
    this->Internals->ActiveRepresentation = cur;
    this->ClearCache();
  }

  this->SomethingUpdated = false;
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkSpreadSheetView::StreamToClient()
{
  vtkSpreadSheetRepresentation* cur = this->Internals->ActiveRepresentation;
  if (cur == NULL)
  {
    if (this->NumberOfRows > 0)
    {
      // BUG #13231: It implies that we had some data in the previous render and
      // we don't have it anymore. We need to trigger "UpdateDataEvent" to
      // ensure that the UI can update the rows and columns correctly.
      this->NumberOfRows = 0;
      this->InvokeEvent(vtkCommand::UpdateDataEvent);
    }
    return 0;
  }

  vtkTypeUInt64 num_rows = 0;

  // From the active representation obtain the data/selection producers that
  // need to be streamed to the client.
  vtkAlgorithmOutput* dataPort = vtkGetDataProducer(this, cur);
  //  vtkAlgorithmOutput* selectionPort = vtkGetSelectionProducer(this, cur);

  this->TableSelectionMarker->SetInputConnection(0, dataPort);
  this->TableSelectionMarker->SetInputConnection(1, cur->GetExtractedDataProducer());
  this->TableStreamer->SetInputConnection(this->TableSelectionMarker->GetOutputPort());
  if (dataPort)
  {
    dataPort->GetProducer()->Update();
    this->DeliveryFilter->SetInputConnection(this->ReductionFilter->GetOutputPort());
    num_rows =
      vtkCountNumberOfRows(dataPort->GetProducer()->GetOutputDataObject(dataPort->GetIndex()));
  }
  else
  {
    this->DeliveryFilter->RemoveAllInputs();
  }

  this->AllReduce(num_rows, num_rows, vtkCommunicator::SUM_OP);

  if (this->NumberOfRows != static_cast<vtkIdType>(num_rows))
  {
    this->SomethingUpdated = true;
  }
  this->NumberOfRows = num_rows;
  if (this->SomethingUpdated)
  {
    this->ClearCache();
    this->InvokeEvent(vtkCommand::UpdateDataEvent);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::OnRepresentationUpdated()
{
  this->ClearCache();
  this->SomethingUpdated = true;
}

//----------------------------------------------------------------------------
vtkTable* vtkSpreadSheetView::FetchBlock(vtkIdType blockindex)
{
  vtkTable* block = this->Internals->GetDataObject(blockindex);
  if (!block)
  {
    block = this->FetchBlockCallback(blockindex);
    this->Internals->AddToCache(blockindex, block, 10);
    this->InvokeEvent(vtkCommand::UpdateEvent, &blockindex);
  }
  return block;
}

//----------------------------------------------------------------------------
vtkTable* vtkSpreadSheetView::FetchBlockCallback(vtkIdType blockindex)
{
  // Sanity Check
  if (!this->Internals->ActiveRepresentation)
  {
    return NULL;
  }

  // cout << "FetchBlockCallback" << endl;
  vtkTypeUInt64 data[2] = { this->Identifier, static_cast<vtkTypeUInt64>(blockindex) };
  if (auto dController = this->GetSession()->GetController(vtkPVSession::DATA_SERVER_ROOT))
  {
    dController->TriggerRMIOnAllChildren(data, sizeof(vtkTypeUInt64) * 2, FETCH_BLOCK_TAG);
  }
  auto pController = vtkMultiProcessController::GetGlobalController();
  if (pController && pController->GetLocalProcessId() == 0 &&
    pController->GetNumberOfProcesses() > 1)
  {
    pController->TriggerRMIOnAllChildren(data, sizeof(vtkTypeUInt64) * 2, FETCH_BLOCK_TAG);
  }

  this->TableStreamer->SetBlock(blockindex);
  this->TableStreamer->Modified();
  this->TableSelectionMarker->SetFieldAssociation(this->FieldAssociation);
  this->ReductionFilter->Modified();
  this->DeliveryFilter->Modified();
  this->DeliveryFilter->Update();
  return vtkTable::SafeDownCast(this->DeliveryFilter->GetOutput());
}

//----------------------------------------------------------------------------
vtkIdType vtkSpreadSheetView::GetNumberOfColumns()
{
  return this->Internals->GetNumberOfColumns(this);
}

//----------------------------------------------------------------------------
vtkIdType vtkSpreadSheetView::GetNumberOfRows()
{
  return this->NumberOfRows;
}

//----------------------------------------------------------------------------
const char* vtkSpreadSheetView::GetColumnName(vtkIdType index)
{
  return this->Internals->GetColumnName(index, this);
}

//----------------------------------------------------------------------------
std::string vtkSpreadSheetView::GetColumnLabel(vtkIdType index)
{
  return this->GetColumnLabel(this->GetColumnName(index));
}

//----------------------------------------------------------------------------
std::string vtkSpreadSheetView::GetColumnLabel(const char* name)
{
  if (name == nullptr || this->IsColumnInternal(name))
  {
    return std::string();
  }

  bool cleaned = false;
  auto cleanedname = ::get_userfriendly_name(name, this, &cleaned);
  return cleaned ? cleanedname : this->Internals->GetOriginalArrayName(name);
}

//----------------------------------------------------------------------------
vtkVariant vtkSpreadSheetView::GetValue(vtkIdType row, vtkIdType col)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  return block->GetValue(blockOffset, col);
}

//----------------------------------------------------------------------------
vtkVariant vtkSpreadSheetView::GetValueByName(vtkIdType row, const char* columnName)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  return block->GetValueByName(blockOffset, columnName);
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsRowSelected(vtkIdType row)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  vtkCharArray* vtkIsSelected =
    vtkCharArray::SafeDownCast(block->GetColumnByName("__vtkIsSelected__"));
  if (vtkIsSelected)
  {
    return vtkIsSelected->GetValue(blockOffset) == 1;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsAvailable(vtkIdType row)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  return this->Internals->GetDataObject(blockIndex) != NULL;
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::Export(vtkCSVExporter* exporter)
{
  if (!exporter->Open())
  {
    return false;
  }
  this->ClearCache();

  if (auto activeRepr = this->Internals->ActiveRepresentation)
  {
    vtkIdType blockSize = this->TableStreamer->GetBlockSize();
    vtkIdType numBlocks = (this->GetNumberOfRows() / blockSize) + 1;
    for (vtkIdType cc = 0; cc < numBlocks; cc++)
    {
      if (vtkTable* block = this->FetchBlock(cc))
      {
        auto* rowData = block->GetRowData();
        if (cc == 0)
        {
          // update column labels; this ensures that all the columns have same
          // names as the spreadsheet view.
          for (vtkIdType idx = 0; idx < rowData->GetNumberOfArrays(); ++idx)
          {
            if (auto array = rowData->GetAbstractArray(idx))
            {
              // note: internal columns get nullptr label which the exporter
              // skips.
              auto name = array->GetName();
              auto label = this->GetColumnLabel(name);
              if (this->IsColumnInternal(name) || this->IsColumnHiddenByName(name) ||
                this->IsColumnHiddenByLabel(label))
              {
                exporter->SetColumnLabel(array->GetName(), nullptr);
              }
              else
              {
                // we don't use the label since it is same for all components in
                // the array, instead we only use user friendly version of the
                // array name.
                exporter->SetColumnLabel(array->GetName(), ::get_userfriendly_name(name, this));
              }
            }
          }
          exporter->WriteHeader(rowData);
        }
        exporter->WriteData(rowData);
      }
    }
  }
  exporter->Close();
  return true;
}

//***************************************************************************
// Forwarded to vtkSortedTableStreamer.
//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetColumnNameToSort(const char* name)
{
  this->TableStreamer->SetColumnNameToSort(name);
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetInvertSortOrder(bool val)
{
  this->TableStreamer->SetInvertOrder(val ? 1 : 0);
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetBlockSize(vtkIdType val)
{
  this->TableStreamer->SetBlockSize(val);
  this->ClearCache();
}
