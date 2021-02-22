/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextViewDataDeliveryManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContextViewDataDeliveryManager.h"
#include "vtkPVDataDeliveryManagerInternals.h"

#include "vtkClientServerMoveData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVView.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionDeliveryFilter.h"
#include "vtkSelectionSerializer.h"

#include <cassert>

vtkStandardNewMacro(vtkPVContextViewDataDeliveryManager);
//----------------------------------------------------------------------------
vtkPVContextViewDataDeliveryManager::vtkPVContextViewDataDeliveryManager() = default;

//----------------------------------------------------------------------------
vtkPVContextViewDataDeliveryManager::~vtkPVContextViewDataDeliveryManager() = default;

//----------------------------------------------------------------------------
void vtkPVContextViewDataDeliveryManager::MoveData(
  vtkPVDataRepresentation* repr, bool low_res, int port)
{
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    // nothing to do on render server nodes.
    // we don't yet support tile-display or CAVE in this mode and hence there's
    // no need to support any rendering of charts on the render server nodes for
    // now.
    return;
  }

  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res, port);
  const auto cacheKey = this->GetCacheKey(repr);

  const bool in_tile_display_mode = this->GetView()->InTileDisplayMode();
  auto pm = vtkProcessModule::GetProcessModule();
  auto controller = pm->GetGlobalController();

  auto dobj = item->GetDataObject(cacheKey);
  assert(dobj);

  if (vtkSelection::SafeDownCast(dobj))
  {
    vtkNew<vtkSelectionDeliveryFilter> courier;
    courier->SetInputDataObject(dobj);
    courier->Update();

    vtkSmartPointer<vtkSelection> selection =
      vtkSelection::SafeDownCast(courier->GetOutputDataObject(0));
    assert(selection);

    if (pm->GetNumberOfLocalPartitions() > 1 && in_tile_display_mode)
    {
      if (pm->GetPartitionId() == 0)
      {
        std::ostringstream res;
        vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, selection);

        // Send the size of the string.
        int size = static_cast<int>(res.str().size());
        controller->Broadcast(&size, 1, 0);

        // Send the XML string.
        controller->Broadcast(const_cast<char*>(res.str().c_str()), size, 0);
      }
      else
      {
        int size = 0;
        controller->Broadcast(&size, 1, 0);
        char* xml = new char[size + 1];

        // Get the string itself.
        controller->Broadcast(xml, size, 0);
        xml[size] = 0;

        // Parse the XML.
        selection->Initialize();
        vtkSelectionSerializer::Parse(xml, selection);
        delete[] xml;
      }
    }
    item->SetDeliveredDataObject(this->GetDeliveredDataKey(low_res), cacheKey, selection);
  }
  else
  {
    if (pm->GetNumberOfLocalPartitions() > 1 && in_tile_display_mode)
    {
      controller->Broadcast(dobj, 0);
    }

    if (pm->GetPartitionId() == 0)
    {
      vtkNew<vtkClientServerMoveData> mover;
      mover->SetInputData(dobj);
      mover->SetOutputDataType(dobj->GetDataObjectType());
      mover->Update();
      item->SetDeliveredDataObject(
        this->GetDeliveredDataKey(low_res), cacheKey, mover->GetOutputDataObject(0));
    }
    else
    {
      item->SetDeliveredDataObject(this->GetDeliveredDataKey(low_res), cacheKey, dobj);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVContextViewDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
