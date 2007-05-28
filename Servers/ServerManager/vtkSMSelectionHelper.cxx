/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionHelper.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVSelectionInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkSMSelectionHelper);
vtkCxxRevisionMacro(vtkSMSelectionHelper, "1.4");

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::AddSourceIDs(vtkSelection* sel,
                                        vtkSMRenderModuleProxy* rmp)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; ++cc)
    {
    vtkSMSelectionHelper::AddSourceIDs(sel->GetChild(cc), rmp);
    }

  vtkInformation* properties = sel->GetProperties();
  if (!properties->Has(vtkSelection::PROP_ID()))
    {
    return;
    }

  int propIdInt = properties->Get(vtkSelection::PROP_ID());

  vtkClientServerID propId;
  propId.ID = propIdInt;

  // get the proxies corresponding to the picked display
  // proxy
  vtkSMProxy* objP = 
    rmp->GetProxyFromPropID(&propId, vtkSMRenderModuleProxy::INPUT);
  vtkSMProxy* geomP = 
    rmp->GetProxyFromPropID(&propId, vtkSMRenderModuleProxy::GEOMETRY);

  if (geomP)
    {
    properties->Set(vtkSelection::SOURCE_ID(), geomP->GetID().ID);
    }

  if (objP)
    {
    if (vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(objP))
      {
      // For compound proxies, the selected proxy is the consumed proxy.
      properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
        cp->GetConsumableProxy()->GetID().ID);
      }
    else
      {
      properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
        objP->GetID().ID);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::SendSelection(vtkSelection* sel, vtkSMProxy* proxy)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  ostrstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);
  res << ends;
  vtkClientServerStream stream;
  vtkClientServerID parserID =
    processModule->NewStreamObject("vtkSelectionSerializer", stream);
  stream << vtkClientServerStream::Invoke
         << parserID << "Parse" << res.str() << proxy->GetID()
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(parserID, stream);

  processModule->SendStream(proxy->GetConnectionID(), 
    proxy->GetServers(), 
    stream);
  delete[] res.str();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::SelectOnSurface(vtkSMRenderModuleProxy* rmP,
                                           int rectangle[4],
                                           vtkCollection* selectedProxies,
                                           vtkCollection* selections)
{
  selectedProxies->RemoveAllItems();
  selections->RemoveAllItems();

  // Select the visible cells using g-buffer selection.
  vtkSelection *selection = rmP->SelectVisibleCells(
    rectangle[0], rectangle[1],
    rectangle[2], rectangle[3]);

  // Make sure SOURCE_ID() and ORIGINAL_SOURCE_ID() are also available in
  // the selection.
  vtkSMSelectionHelper::AddSourceIDs(selection, rmP);

  // Create a set of prop ids (no duplications)
  vtkstd::set<int> propIDs;
  unsigned int numChildren = selection->GetNumberOfChildren();
  unsigned int i;
  int MaxPixels = -1;
  int PropWithMostPixels = -1;
  for (i=0; i<numChildren; i++)
    {
    vtkSelection* child = selection->GetChild(i);
    vtkInformation* properties = child->GetProperties();
    int NumPixels = 0;
    if (properties->Has(vtkSelection::PIXEL_COUNT()))
      {
      NumPixels = properties->Get(vtkSelection::PIXEL_COUNT());
      if (NumPixels > MaxPixels)
        {
        if (properties->Has(vtkSelection::PROP_ID()))
          {
          PropWithMostPixels = properties->Get(vtkSelection::PROP_ID());
          MaxPixels = NumPixels;
          }        
        }
      }
    }

  if (PropWithMostPixels >= 0)
    {
    propIDs.insert(PropWithMostPixels);

    // For each item in the set, find the corresponding proxy
    // and selection
    vtkstd::set<int>::iterator iter = propIDs.begin();
    for(; iter != propIDs.end(); iter++)
      {
      vtkClientServerID propID;
      propID.ID = *iter;
      vtkSMProxy* objP = 
        rmP->GetProxyFromPropID(&propID, vtkSMRenderModuleProxy::INPUT);
      if (objP)
        {
        selectedProxies->AddItem(objP);
        vtkSelection* newSelection = vtkSelection::New();
        newSelection->GetProperties()->Copy(selection->GetProperties(), 0);
        for(i=0; i<numChildren; i++)
          {
          vtkSelection* child = selection->GetChild(i);
          vtkInformation* properties = child->GetProperties();
          if (properties->Has(vtkSelection::PROP_ID()) &&
              properties->Get(vtkSelection::PROP_ID()) == (int)propID.ID)
            {
            vtkSelection* newChildSelection = vtkSelection::New();
            newChildSelection->ShallowCopy(child);
            newSelection->AddChild(newChildSelection);
            newChildSelection->Delete();
            }
          }
        selections->AddItem(newSelection);
        newSelection->Delete();
        }
      }
    }

  selection->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 0);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToGlobalIDVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input, vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 1);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output,
  int global_ids)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  // Convert the selection of geometry cells to surface cells of the actual
  // data by:

  // * First sending the selection to the server
  vtkSMProxy* selectionP = pxm->NewProxy("selection_helpers", "Selection");
  selectionP->SetServers(vtkProcessModule::DATA_SERVER);
  selectionP->SetConnectionID(connectionID);
  selectionP->UpdateVTKObjects();
  vtkSMSelectionHelper::SendSelection(input, selectionP);

  // * Then converting the selection using a helper
  vtkSMProxy* volumeSelectionP = 
    pxm->NewProxy("selection_helpers", "Selection");
  volumeSelectionP->SetServers(vtkProcessModule::DATA_SERVER);
  volumeSelectionP->SetConnectionID(connectionID);
  volumeSelectionP->UpdateVTKObjects();
  vtkClientServerStream stream;
  vtkClientServerID converterID =
    processModule->NewStreamObject("vtkSelectionConverter", stream);
  stream << vtkClientServerStream::Invoke
         << converterID 
         << "Convert" 
         << selectionP->GetID() 
         << volumeSelectionP->GetID()
         << global_ids
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(connectionID, 
                            vtkProcessModule::DATA_SERVER, 
                            stream);

  // * And finally gathering the information back
  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  processModule->GatherInformation(connectionID,
                                   vtkProcessModule::DATA_SERVER, 
                                   selInfo, 
                                   volumeSelectionP->GetID());

  output->ShallowCopy(selInfo->GetSelection());

  selInfo->Delete();
  volumeSelectionP->Delete();
  selectionP->Delete();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelection(
  vtkIdType connectionID,
  vtkSelection* selection)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // Now pass the selection ids to a selection source.
  vtkSMProxy* selectionSourceP = pxm->NewProxy("sources", "SelectionSource");
  selectionSourceP->SetConnectionID(connectionID);
  selectionSourceP->SetServers(vtkProcessModule::DATA_SERVER);

  unsigned int numChildren = selection->GetNumberOfChildren();
  unsigned int childId;
  vtkIdType numIDs=0;

  // Count the total number of ids over. all children
  for(childId=0; childId< numChildren; childId++)
    {
    vtkSelection* child = selection->GetChild(childId);
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      child->GetSelectionList());
    if (idList)
      {
      numIDs += idList->GetNumberOfTuples();
      }
    }

  // Add the selection proc ids and cell ids to the IDs property.
  vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("IDs"));
  ids->SetNumberOfElements(numIDs*2);

  vtkIdType counter = 0;
  for(childId=0; childId< numChildren; childId++)
    {
    vtkSelection* child = selection->GetChild(childId);
    int procID = 0;
    if (child->GetProperties()->Has(vtkSelection::PROCESS_ID()))
      {
      procID = child->GetProperties()->Get(vtkSelection::PROCESS_ID());
      }
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      child->GetSelectionList());
    if (idList)
      {
      vtkIdType numValues = idList->GetNumberOfTuples();
      for (vtkIdType idx=0; idx<numValues; idx++)
        {
        ids->SetElement(counter++, procID);
        ids->SetElement(counter++, idList->GetValue(idx));
        }
      }
    }

  /*
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("ContentType"));
  ivp->SetElement(0, selection->GetProperties()->Get(vtkSelection::CONTENT_TYPE()));

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceP->GetProperty("FieldType"));
  ivp->SetElement(0, selection->GetProperties()->Get(vtkSelection::FIELD_TYPE()));
  */

  selectionSourceP->UpdateVTKObjects();
  return selectionSourceP;
}
