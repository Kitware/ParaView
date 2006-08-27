/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVSelectionInformation.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMPropertyIterator.h"

#include <vtkstd/list>
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
static void vtkSMSelectionProxyReorderBoundingBox(int src[4], int dest[4])
{
  dest[0] = (src[0] < src[2])? src[0] : src[2];
  dest[1] = (src[1] < src[3])? src[1] : src[3];
  dest[2] = (src[0] < src[2])? src[2] : src[0];
  dest[3] = (src[1] < src[3])? src[3] : src[1];
}

//-----------------------------------------------------------------------------
// traverse selection and extract prop ids (unique)
static void vtkSMSelectionProxyExtractPropIds(
  vtkSelection* sel, vtkstd::list<int>& ids)
{
  vtkInformation* properties = sel->GetProperties();
  if (properties && properties->Has(vtkSelection::PROP_ID()))
    {
    int id = properties->Get(vtkSelection::PROP_ID());
    if (vtkstd::find(ids.begin(), ids.end(), id) == ids.end())
      {
      ids.push_back(id);
      }
    }
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSMSelectionProxyExtractPropIds(sel->GetChild(i), ids);
    }
}

vtkStandardNewMacro(vtkSMSelectionProxy);
vtkCxxRevisionMacro(vtkSMSelectionProxy, "1.1");
vtkCxxSetObjectMacro(vtkSMSelectionProxy, RenderModule, vtkSMRenderModuleProxy);
vtkCxxSetObjectMacro(vtkSMSelectionProxy, ClientSideSelection, vtkSelection);
//-----------------------------------------------------------------------------
vtkSMSelectionProxy::vtkSMSelectionProxy()
{
  this->Mode = vtkSMSelectionProxy::SURFACE;
  this->Type = vtkSMSelectionProxy::SOURCE;
  this->RenderModule = 0;
  this->ClientSideSelection = 0;
  this->SelectionUpToDate = 0;
}

//-----------------------------------------------------------------------------
vtkSMSelectionProxy::~vtkSMSelectionProxy()
{
  this->SetRenderModule(0);
  this->SetClientSideSelection(0);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::GetSelectedSourceProxies(vtkCollection* coll)
{
  if (!this->ObjectsCreated || this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Selection Proxy not created yet.");
    return; 
    }
  if (!coll)
    {
    return;
    }

  if (!this->ClientSideSelection)
    {
    // gather the selection results from the server.
    vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

    vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
    processModule->GatherInformation(
      this->GetConnectionID(),
      vtkProcessModule::RENDER_SERVER, 
      selInfo, 
      this->GetID(0)
    );
    this->SetClientSideSelection(selInfo->GetSelection());
    selInfo->Delete();
    }
  this->FillSources(this->ClientSideSelection, this->RenderModule, coll);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  vtkSMProxy* cameraProxy = this->GetSubProxy("Camera");
  if (!cameraProxy)
    {
    vtkErrorMacro("Camera subproxy must be defined in the configuration.");
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::UpdateSelection()
{
  this->SetClientSideSelection(0);
  if (!this->RenderModule)
    {
    vtkErrorMacro("RenderModule is required to update selection.");
    return;
    }

  if (this->RenderModule->GetConnectionID() != this->GetConnectionID())
    {
    vtkErrorMacro("RenderModule and Selection cannot be on different servers.");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent);

  this->UpdateRenderModuleCamera(false);

  vtkSMProxy* geomSelectionProxy = this;
  if (this->Type != vtkSMSelectionProxy::GEOMETRY)
    {
    geomSelectionProxy = this->GetProxyManager()->NewProxy(
      "selection_helpers", "Selection");
    geomSelectionProxy->SetConnectionID(this->GetConnectionID());
    geomSelectionProxy->SetServers(this->GetServers());
    geomSelectionProxy->UpdateVTKObjects();
    }

  switch (this->Mode)
    {
  case vtkSMSelectionProxy::SURFACE:
    this->SelectOnSurface(this->Selection, this->RenderModule, geomSelectionProxy);
    break;

  case vtkSMSelectionProxy::FRUSTRUM:
    this->SelectInFrustrum(this->Selection, this->RenderModule, geomSelectionProxy);
    break;
    }

  if (this->Type != vtkSMSelectionProxy::GEOMETRY)
    {
    // We need to convert the geometry selection to source selection.
    this->ConvertGeometrySelectionToSource(geomSelectionProxy, this);
    geomSelectionProxy->Delete();
    }

  this->UpdateRenderModuleCamera(true);

  this->SelectionUpToDate = 1;
  this->InvokeEvent(vtkCommand::EndEvent);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectOnSurface(int in_rect[4],
  vtkSMRenderModuleProxy* renderModule, vtkSMProxy* geomSelectionProxy)
{
  int display_rectangle[4];
  vtkSMSelectionProxyReorderBoundingBox(in_rect, display_rectangle);

  vtkSelection *selection = renderModule->SelectVisibleCells(
    display_rectangle[0], display_rectangle[1],
    display_rectangle[2], display_rectangle[3]);

  // The selection returned by renderModule is not complete since it does not
  // have information about ORIGINAL_SOURCE_ID. Hence we convert the selection.
  this->ConvertSelection(selection, renderModule);

  // Since the selection in on the client, we need to send it to the data server.
  this->SendSelection(selection, geomSelectionProxy);

  // Since we already have a client side selection, fill up the selected source
  // collection, just in case the user needs it.
  this->SetClientSideSelection(selection);
  selection->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::ConvertSelection(vtkSelection* sel,
  vtkSMRenderModuleProxy* rmp)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; ++cc)
    {
    this->ConvertSelection(sel->GetChild(cc), rmp);
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
  vtkSMProxy* objProxy = 
    rmp->GetProxyFromPropID(&propId, vtkSMRenderModuleProxy::INPUT);
  vtkSMProxy* geomProxy = 
    rmp->GetProxyFromPropID(&propId, vtkSMRenderModuleProxy::GEOMETRY);

  properties->Set(vtkSelection::SOURCE_ID(), geomProxy->GetID(0).ID);
  properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
                  objProxy->GetID(0).ID);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SendSelection(vtkSelection* sel, vtkSMProxy* proxy)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  ostrstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);
  res << ends;
  vtkClientServerStream stream;
  vtkClientServerID parserID =
    processModule->NewStreamObject("vtkSelectionSerializer", stream);
  stream << vtkClientServerStream::Invoke
    << parserID << "Parse" << res.str() << proxy->GetID(0)
    << vtkClientServerStream::End;
  processModule->DeleteStreamObject(parserID, stream);

  processModule->SendStream(proxy->GetConnectionID(), 
    proxy->GetServers(), 
    stream);
  delete[] res.str();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::ConvertGeometrySelectionToSource(
  vtkSMProxy* geomSelectionProxy, vtkSMProxy* sourceSelectionProxy)
{
  if (!geomSelectionProxy || !sourceSelectionProxy)
    {
    return;
    }

  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID converterID =
    processModule->NewStreamObject("vtkSelectionConverter", stream);
  stream << vtkClientServerStream::Invoke
    << converterID 
    << "Convert" 
    << geomSelectionProxy->GetID(0) 
    << sourceSelectionProxy->GetID(0)
    << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(sourceSelectionProxy->GetConnectionID(), 
    sourceSelectionProxy->GetServers(), stream);
}


//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectInFrustrum(int in_rect[4],
  vtkSMRenderModuleProxy* rmp, vtkSMProxy* geomSelectionProxy)
{
  vtkRenderer *renderer = rmp->GetRenderer();

  int display_rectangle[4];
  vtkSMSelectionProxyReorderBoundingBox(in_rect, display_rectangle);

  // First, compute the frustrum from the given rectangle
  double x0 = display_rectangle[0];
  double y0 = display_rectangle[1];
  double x1 = display_rectangle[2];
  double y1 = display_rectangle[3];

  if (x0 == x1)
    {
    x0 -= 0.5;
    x1 += 0.5;
    }
  if (y0 == y1)
    {
    y0 -= 0.5;
    y1 += 0.5;
    }

  double frustrum[32];
  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[0]);

  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[4]);

  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[8]);

  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[12]);

  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[16]);

  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[20]);

  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[24]);

  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustrum[28]);

  // pick with the given rectange. this will need some work when rendering
  // in parallel, specially with tiled display
  vtkPVClientServerIdCollectionInformation* idInfo = 
    rmp->Pick(
      static_cast<int>(x0), 
      static_cast<int>(y0), 
      static_cast<int>(x1), 
      static_cast<int>(y1));

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > geomFilters;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > sources;

  int numProps = idInfo->GetLength();
  for (int i = 0; i < numProps; i++)
    {
    vtkClientServerID id = idInfo->GetID(i);
    // get the proxies corresponding to the picked display
    // proxy
    vtkSMProxy* objProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::INPUT);
    vtkSMProxy* dispProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::DISPLAY);
    vtkSMProxy* geomProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::GEOMETRY);
    if (!dispProxy)
      {
      continue;
      }
    if (!geomProxy)
      {
      continue;
      }
    // Make sure the proxy is visible and pickable
    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      dispProxy->GetProperty("Pickable"));
    if (enabled)
      {
      if (!enabled->GetElement(0))
        {
        continue;
        }
      }
    enabled = vtkSMIntVectorProperty::SafeDownCast(
      dispProxy->GetProperty("Visibility"));
    if (enabled)
      {
      if (!enabled->GetElement(0))
        {
        continue;
        }
      }
    // selected
    displays.push_back(dispProxy);
    geomFilters.push_back(geomProxy);
    sources.push_back(objProxy);
    }

  // now select cells
  vtkSMProxy* selectorProxy = 
    this->GetProxyManager()->NewProxy("selection_helpers", "VolumeSelector");
  if (!selectorProxy)
    {
    vtkErrorMacro("No selection proxy was created on the data server. "
      "Cannot select cells");
    return;
    }
  selectorProxy->SetConnectionID(rmp->GetConnectionID());
  selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);

  vtkSMProxyProperty* selectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("Selection"));
  selectionProperty->AddProxy(geomSelectionProxy);

  vtkSMDoubleVectorProperty* cf = vtkSMDoubleVectorProperty::SafeDownCast(
    selectorProxy->GetProperty("CreateFrustum"));
  cf->SetElements(&frustrum[0]);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  vtkSMProxyProperty* dataSets = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("DataSets"));
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = geomFilters.size();
  for (unsigned int i = 0; i < numProxies; i++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[i]);
    if (dp)
      {
      dataSets->AddProxy(geomFilters[i]);
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[i]);
      }
    }
  idInfo->Delete();
  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Select");
  // no need for the selector anymore.
  selectorProxy->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::FillSources(vtkSelection* sel, 
  vtkSMRenderModuleProxy* rmp, vtkCollection* coll)
{
  if (!rmp || !sel || !coll)
    {
    return;
    }

  vtkstd::list<int> ids;
  vtkSMSelectionProxyExtractPropIds(sel, ids);

  coll->RemoveAllItems();
  vtkstd::list<int>::iterator iter = ids.begin();
  for(; iter != ids.end(); iter++)
    {
    vtkClientServerID id;
    id.ID = *iter;
    vtkSMProxy* objProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::INPUT);
    coll->AddItem(objProxy); 
    }
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::UpdateCameraPropertiesFromRenderModule()
{
  if (!this->RenderModule)
    {
    vtkErrorMacro("RenderModule must be set before calling "
      "UpdateCameraPropertiesFromRenderModule");
    return;
    }
  this->RenderModule->SynchronizeCameraProperties();
  vtkSMProxy* camera = this->GetSubProxy("Camera");
  if (!camera)
    {
    vtkErrorMacro("Camera subproxy must be defined in the configuration.");
    return;
    }

  vtkSMPropertyIterator* iter = camera->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *cur_property = iter->GetProperty();
    if (cur_property->GetInformationOnly())
      {
      continue;
      }
    vtkSMProperty* src_property = 
      this->RenderModule->GetProperty(iter->GetKey());
    if (src_property && cur_property)
      {
      cur_property->Copy(src_property);
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::UpdateRenderModuleCamera(bool reset)
{
  if (!this->RenderModule)
    {
    vtkErrorMacro("RenderModule must be set before calling "
      "UpdateCameraPropertiesFromRenderModule");
    return;
    }
  vtkSMProxy* camera = this->GetSubProxy("Camera");
  if (!camera)
    {
    vtkErrorMacro("Camera subproxy must be defined in the configuration.");
    return;
    }

  vtkSMProxy* reset_camera = this->GetSubProxy("CameraResetter");
  if (!camera)
    {
    vtkErrorMacro("CameraResetter subproxy must be defined in the configuration.");
    return;
    }

  this->RenderModule->SynchronizeCameraProperties();

  vtkSMPropertyIterator* iter = camera->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *cam_property = iter->GetProperty();
    if (cam_property->GetInformationOnly())
      {
      continue;
      }
    vtkSMProperty* rm_property = 
      this->RenderModule->GetProperty(iter->GetKey());
    vtkSMProperty* reset_property = 
      reset_camera->GetProperty(iter->GetKey());

    if (rm_property && cam_property && reset_property)
      {
      if (reset)
        {
        rm_property->Copy(reset_property);
        }
      else
        {
        reset_property->Copy(rm_property);
        rm_property->Copy(cam_property);
        }
      }
    }
  this->RenderModule->UpdateVTKObjects();
  iter->Delete();
}


//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy == this)
    {
    this->SelectionUpToDate = 0;
    }
  this->Superclass::MarkModified(modifiedProxy);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
