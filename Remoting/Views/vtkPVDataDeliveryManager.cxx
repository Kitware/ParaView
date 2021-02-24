/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataDeliveryManagerInternals.h"

#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVView.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

//*****************************************************************************
//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::vtkPVDataDeliveryManager()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::~vtkPVDataDeliveryManager()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetView(vtkPVView* view)
{
  this->View = view;
}

//----------------------------------------------------------------------------
vtkPVView* vtkPVDataDeliveryManager::GetView() const
{
  return this->View;
}

//----------------------------------------------------------------------------
unsigned long vtkPVDataDeliveryManager::GetVisibleDataSize(bool low_res)
{
  return this->Internals->GetVisibleDataSize(low_res, this);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RegisterRepresentation(vtkPVDataRepresentation* repr)
{
  assert("A representation must have a valid UniqueIdentifier" && repr->GetUniqueIdentifier());
  this->Internals->RepresentationsMap[repr->GetUniqueIdentifier()] = repr;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::UnRegisterRepresentation(vtkPVDataRepresentation* repr)
{
  unsigned int rid = repr->GetUniqueIdentifier();
  this->Internals->RepresentationsMap.erase(rid);

  vtkInternals::ItemsMapType::iterator iter = this->Internals->ItemsMap.begin();
  while (iter != this->Internals->ItemsMap.end())
  {
    const vtkInternals::ReprPortType& key = iter->first;
    if (key.first == rid)
    {
      vtkInternals::ItemsMapType::iterator toerase = iter;
      ++iter;
      this->Internals->ItemsMap.erase(toerase);
    }
    else
    {
      ++iter;
    }
  }
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation* vtkPVDataDeliveryManager::GetRepresentation(unsigned int index)
{
  vtkInternals::RepresentationsMapType::const_iterator iter =
    this->Internals->RepresentationsMap.find(index);
  return iter != this->Internals->RepresentationsMap.end() ? iter->second.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
int vtkPVDataDeliveryManager::GetNumberOfPorts(vtkPVDataRepresentation* repr)
{
  return this->Internals->GetNumberOfPorts(repr);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(vtkPVDataRepresentation* repr, vtkDataObject* data,
  bool low_res, unsigned long trueSize, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/true);
  if (item)
  {
    const auto cacheKey = this->GetCacheKey(repr);
    if (item->GetDataObject(cacheKey) == nullptr ||
      repr->GetPipelineDataTime() > item->GetTimeStamp())
    {
      vtkLogF(
        TRACE, "SetDataObject %s (key=%g) : %p", repr->GetLogName().c_str(), cacheKey, (void*)data);
      item->SetDataObject(data, this->Internals, cacheKey);
      if (trueSize > 0)
      {
        item->SetActualMemorySize(trueSize, cacheKey);
      }

      if (low_res == false)
      {
        // clear low_res data whenever full-res data changes. this ensures that
        // we won't use obsolete low-res data.
        this->SetPiece(repr, nullptr, true, 0, port);
      }
    }
  }
  else
  {
    vtkErrorMacro("Invalid argument.");
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVDataDeliveryManager::GetPiece(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  const auto cacheKey = this->GetCacheKey(repr);
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/false);
  return item ? item->GetDataObject(cacheKey) : nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::HasPiece(vtkPVDataRepresentation* repr, bool low_res, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/false);
  const auto cacheKey = this->GetCacheKey(repr);
  const bool val = item ? (item->GetDataObject(cacheKey) != nullptr) : false;

  vtkLogF(TRACE, "HasPiece %s (key=%g) : %d", repr->GetLogName().c_str(), cacheKey, val);
  return val;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPVDataDeliveryManager::GetDeliveredPiece(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/false);
  const int dataKey = this->GetDeliveredDataKey(low_res);
  const auto cacheKey = this->GetCacheKey(repr);
  return item ? item->GetDeliveredDataObject(dataKey, cacheKey) : nullptr;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkPVDataDeliveryManager::GetProducer(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res, port);
  if (!item)
  {
    vtkErrorMacro("Invalid arguments.");
    return nullptr;
  }

  const int dataKey = this->GetDeliveredDataKey(low_res);
  const auto cacheKey = this->GetCacheKey(repr);
  return item->GetProducer(dataKey, cacheKey)->GetOutputPort(0);
}

//----------------------------------------------------------------------------
vtkInformation* vtkPVDataDeliveryManager::GetPieceInformation(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  vtkInternals::vtkItem* item =
    this->Internals->GetItem(repr, low_res, port, /*create_if_needed=*/true);
  const auto cacheKey = this->GetCacheKey(repr);
  return item ? item->GetPieceInformation(cacheKey) : nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::NeedsDelivery(
  vtkMTimeType timestamp, std::vector<unsigned int>& keys_to_deliver, bool low_res)
{
  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "check for delivery (low_res=%s)",
    (low_res ? "true" : "false"));
  assert(this->View);
  vtkInternals::ItemsMapType::iterator iter;
  for (iter = this->Internals->ItemsMap.begin(); iter != this->Internals->ItemsMap.end(); ++iter)
  {
    if (this->Internals->IsRepresentationVisible(iter->first.first))
    {
      auto repr = this->GetRepresentation(iter->first.first);
      const auto cacheKey = this->GetCacheKey(repr);
      const int dataKey = this->GetDeliveredDataKey(low_res);
      vtkInternals::vtkItem& item = low_res ? iter->second.second : iter->second.first;
      if (item.GetTimeStamp(cacheKey) > timestamp ||
        item.GetDeliveryTimeStamp(dataKey, cacheKey) < item.GetTimeStamp(cacheKey))
      {
        vtkVLogF(
          PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "needs-delivery: %s", repr->GetLogName().c_str());
        // FIXME: convert keys_to_deliver to a vector of pairs.
        keys_to_deliver.push_back(iter->first.first);
        keys_to_deliver.push_back(static_cast<unsigned int>(iter->first.second));
      }
    }
  }
  vtkVLogIfF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), keys_to_deliver.size() == 0, "none");
  return keys_to_deliver.size() > 0;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::Deliver(int low_res, unsigned int size, unsigned int* values)
{
  // This method gets called on all processes with the list of representations
  // to "deliver". We check with the view what mode we're operating in and
  // decide where the data needs to be delivered.
  //
  // Representations can provide overrides, e.g. though the view says data is
  // merely "pass-through", some representation says we need to clone the data
  // everywhere. That makes it critical that this method is called on all
  // processes at the same time to avoid deadlocks and other complications.
  //
  // This method will be implemented in "view-specific" subclasses since how the
  // data is delivered is very view specific.

  assert(size % 2 == 0);

  vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "%s data migration",
    (low_res ? "low-resolution" : "full resolution"));
  for (unsigned int cc = 0; cc < size; cc += 2)
  {
    const unsigned int id = values[cc];
    const int port = static_cast<int>(values[cc + 1]);
    if (auto item = this->Internals->GetItem(id, low_res != 0, port))
    {
      auto repr = this->GetRepresentation(id);
      const auto cacheKey = this->GetCacheKey(repr);
      vtkDataObject* data = item ? item->GetDataObject(cacheKey) : nullptr;
      if (!data)
      {
        // ideally, we want to sync this info between all ranks some other rank
        // doesn't deadlock (esp. in collaboration mode).
        continue;
      }
      vtkVLogScopeF(
        PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "move-data: %s", repr->GetLogName().c_str());
      this->MoveData(repr, low_res != 0, port);
    }
  }
}

//----------------------------------------------------------------------------
int vtkPVDataDeliveryManager::GetSynchronizationMagicNumber()
{
  // The synchronization magic number is used to ensure that both the server and
  // the client know of have identical representations, because if they don't,
  // there may be a mismatch between the states of the two processes and it's
  // best to skip delivery to avoid deadlocks.
  const int prime = 31;
  int result = 1;
  result = prime * result + static_cast<int>(this->Internals->RepresentationsMap.size());
  for (auto iter = this->Internals->RepresentationsMap.begin();
       iter != this->Internals->RepresentationsMap.end(); ++iter)
  {
    result = prime * result + static_cast<int>(iter->first);
  }
  return result;
}

//----------------------------------------------------------------------------
double vtkPVDataDeliveryManager::GetCacheKey(vtkPVDataRepresentation* repr) const
{
  assert(repr != nullptr);
  return repr->GetCacheKey();
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::ClearCache(vtkPVDataRepresentation* repr)
{
  this->Internals->ClearCache(repr);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
