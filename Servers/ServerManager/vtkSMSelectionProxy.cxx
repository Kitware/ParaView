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
#include "vtkSMCompoundProxy.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"

#include <vtkstd/algorithm>
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
// traverse selection and extract a linear set of prop_ids without duplicates
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
vtkCxxRevisionMacro(vtkSMSelectionProxy, "1.12");
vtkCxxSetObjectMacro(vtkSMSelectionProxy, RenderModule, vtkSMRenderModuleProxy);
vtkCxxSetObjectMacro(vtkSMSelectionProxy, ClientSideSelection, vtkSelection);
//-----------------------------------------------------------------------------
vtkSMSelectionProxy::vtkSMSelectionProxy()
{
  this->Mode = vtkSMSelectionProxy::SURFACE;
  this->RenderModule = 0;
  this->ClientSideSelection = 0;
  this->SelectionUpToDate = 0;
  this->ScreenRectangle[0] = 0;
  this->ScreenRectangle[1] = 0;
  this->ScreenRectangle[2] = 0;
  this->ScreenRectangle[3] = 0;
  this->NumIds = 0;
  this->Ids = NULL;
  this->NumPoints = 0;
  this->Points = NULL;
  this->NumThresholds = 0;
  this->Thresholds = NULL;
}

//-----------------------------------------------------------------------------
vtkSMSelectionProxy::~vtkSMSelectionProxy()
{
  this->SetRenderModule(0);
  this->SetClientSideSelection(0);
  if (this->Ids != NULL)
    {
    delete[] this->Ids;
    }
  if (this->Points != NULL)
    {
    delete[] this->Points;
    }
  if (this->Thresholds != NULL)
    {
    delete[] this->Thresholds;
    }
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

  vtkSMProxy* inSel = this->GetProxyManager()->NewProxy(
    "selection_helpers", "Selection");
  inSel->SetConnectionID(this->GetConnectionID());
  inSel->SetServers(this->GetServers());
  inSel->UpdateVTKObjects();

  vtkSMProxy* outSel = this;
  if (this->Mode == vtkSMSelectionProxy::SURFACE)
    {
    //doing a surface selection,
    //make a temporary vtkSelection to store ids in
    outSel = this->GetProxyManager()->NewProxy(
      "selection_helpers", "Selection");
    outSel->SetConnectionID(this->GetConnectionID());
    outSel->SetServers(this->GetServers());
    outSel->UpdateVTKObjects();
    }

  switch (this->Mode)
    {
  case vtkSMSelectionProxy::SURFACE:
    this->SelectOnSurface(this->RenderModule, outSel);
    break;

  case vtkSMSelectionProxy::FRUSTUM:
    this->SelectInFrustum(this->RenderModule, inSel, outSel);
    break;

  case vtkSMSelectionProxy::IDS:
    this->SelectIds(this->RenderModule, inSel, outSel);
    break;

  case vtkSMSelectionProxy::POINTS:
    this->SelectPoints(this->RenderModule, inSel, outSel);
    break;

  case vtkSMSelectionProxy::THRESHOLDS:
    this->SelectThresholds(this->RenderModule, inSel, outSel);
    break;

    }

  if (this->Mode == vtkSMSelectionProxy::SURFACE)
    {
    //convert the temporary vtkSelection with suface cell ids
    //into one with volume cell ids
    this->ConvertPolySelectionToVoxelSelection(outSel, this);
    outSel->Delete();
    }

  inSel->Delete();

  this->UpdateRenderModuleCamera(true);

  this->SelectionUpToDate = 1;
  this->InvokeEvent(vtkCommand::EndEvent);

  // Since selection changed, we mark all consumers modified.
  this->MarkConsumersAsModified(0);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectOnSurface(
  vtkSMRenderModuleProxy* renderModule, vtkSMProxy* outSel)
{
  int display_rectangle[4];
  vtkSMSelectionProxyReorderBoundingBox(this->ScreenRectangle, display_rectangle);

  vtkSelection *selection = renderModule->SelectVisibleCells(
    display_rectangle[0], display_rectangle[1],
    display_rectangle[2], display_rectangle[3]);

  // The selection returned by renderModule is not complete since it does not
  // have information about ORIGINAL_SOURCE_ID. Hence we convert the selection.
  this->ConvertSelection(selection, renderModule);

  // Since the selection in on the client, we need to send it to the data server.
  this->SendSelection(selection, outSel);

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

  if (geomProxy)
    {
    properties->Set(vtkSelection::SOURCE_ID(), geomProxy->GetID(0).ID);
    }

  if (objProxy)
    {
    if (vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(objProxy))
      {
      // For compound proxies, the selected proxy is the consumed proxy.
      properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
        cp->GetConsumableProxy()->GetID(0).ID);
      }
    else
      {
      properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
        objProxy->GetID(0).ID);
      }
    }
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
void vtkSMSelectionProxy::ConvertPolySelectionToVoxelSelection(
  vtkSMProxy* outSel, vtkSMProxy* insideSelectionDestination)
{
  if (!outSel || !insideSelectionDestination)
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
    << outSel->GetID(0) 
    << insideSelectionDestination->GetID(0)
    << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(insideSelectionDestination->GetConnectionID(), 
    insideSelectionDestination->GetServers(), stream);
}


//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectInFrustum(
  vtkSMRenderModuleProxy* rmp, vtkSMProxy* inSel, vtkSMProxy* outSel)
{
  vtkRenderer *renderer = rmp->GetRenderer();

  int display_rectangle[4];
  vtkSMSelectionProxyReorderBoundingBox(this->ScreenRectangle, display_rectangle);

  // First, compute the frustum from the given rectangle
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

  double frustum[32];
  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[0]);

  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[4]);

  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[8]);

  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[12]);

  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[16]);

  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[20]);

  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[24]);

  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[28]);

  // pick with the given rectange. this will need some work when rendering
  // in parallel, specially with tiled display
  // if it turns out that area pick doesn't work in some cases, we can 
  // probably just get all visible and pickable objects like the other 
  // extractors do. if we don't have very many datasets being diplayed at the
  // same time then this is not speeding us up anyway.
  vtkPVClientServerIdCollectionInformation* idInfo = 
    rmp->Pick(
      static_cast<int>(x0), 
      static_cast<int>(y0), 
      static_cast<int>(x1), 
      static_cast<int>(y1));

  //create lists that contain the proxies for each picked prop's
  //display, surface filter, and filter that produced what the prop shows

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
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
    if (!dispProxy)
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
    // selectable
    displays.push_back(dispProxy);
    sources.push_back(objProxy);
    }

  //create the extraction filter on the server that will find the results
  vtkSMProxy* selectorProxy = 
    this->GetProxyManager()->NewProxy("selection_helpers", "ExtractSelection");
  if (!selectorProxy)
    {
    vtkErrorMacro("No selection proxy was created on the data server. "
      "Cannot select cells");
    return;
    }
  selectorProxy->SetConnectionID(rmp->GetConnectionID());
  selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);

  //tell the filter what type of extraction to do
  vtkSelection *sel = vtkSelection::New();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::FRUSTUM
    );
  vtkDoubleArray* vals = vtkDoubleArray::New();
  vals->SetNumberOfComponents(1);
  vals->SetNumberOfTuples(32);
  for (int i = 0; i<32; i++)
    {
    vals->SetValue(i, frustum[i]);
    }
  sel->SetSelectionList(vals);
  this->SendSelection(sel,inSel);
  vals->Delete();
  sel->Delete();

  vtkSMProxyProperty* inSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("InputSelection"));
  inSelectionProperty->AddProxy(inSel);

  //tell the filter to put its results where we can get them
  vtkSMProxyProperty* outSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("OutputSelection"));
  outSelectionProperty->AddProxy(outSel);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  //give the extraction filter the list of source algorithms and vtkProps
  //it will grab datasets to extract from the sources
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = displays.size();
  for (unsigned int j = 0; j < numProxies; j++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[j]);
    if (dp)
      {
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[j]);
      }
    }
  idInfo->Delete();

  //tell the extraction filter to execute
  //this will finally select cells
  //the extraction filter produces:
  //for each processor and each dataset
  //  an array of cell ids extracted from the dataset
  //  the client server id of the filter that produced the dataset (SOURCE_ID)
  //  the client server id of the prop that we that we said produced the dataset (PROP_ID)
  //  the rank of the processor that extracted each portion
  // this is all gathered into outSel
  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Select");

  // no need for the selector anymore.
  selectorProxy->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectIds(
  vtkSMRenderModuleProxy* rmp, vtkSMProxy* inSel, vtkSMProxy* outSel)
{  
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > sources;

  // unlike frustum, we don't do a pick first, so we just look for all
  // visible and selectable display objects instead.
  int numProps = rmp->GetDisplays()->GetNumberOfItems();
  for (int i = 0; i < numProps; i++)
    {
    vtkSMProxy* objProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::INPUT);
    vtkSMProxy* dispProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::DISPLAY);
    if (!dispProxy)
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
    // selectable
    displays.push_back(dispProxy);
    sources.push_back(objProxy);
    }

  //create the extraction filter on the server that will find the results
  vtkSMProxy* selectorProxy = 
    this->GetProxyManager()->NewProxy("selection_helpers", "ExtractSelection");
  if (!selectorProxy)
    {
    vtkErrorMacro("No selection proxy was created on the data server. "
      "Cannot select cells");
    return;
    }
  selectorProxy->SetConnectionID(rmp->GetConnectionID());
  selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);

  //tell the filter what type of extraction to do
  vtkSelection *sel = vtkSelection::New();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::CELL_IDS
    );
  vtkIdTypeArray* vals = vtkIdTypeArray::New();
  vals->SetNumberOfComponents(1);
  vals->SetNumberOfTuples(this->NumIds);
  for (int i = 0; i<this->NumIds; i++)
    {
    vals->SetTupleValue(i, &this->Ids[i]);
    }
  sel->SetSelectionList(vals);
  this->SendSelection(sel,inSel);
  vals->Delete();
  sel->Delete();
  
  vtkSMProxyProperty* inSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("InputSelection"));
  inSelectionProperty->AddProxy(inSel);

  //tell the filter to put its results where we can get them
  vtkSMProxyProperty* outSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("OutputSelection"));
  outSelectionProperty->AddProxy(outSel);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  //give the extraction filter the list of source algorithms and vtkProps
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = displays.size();
  for (unsigned int j = 0; j < numProxies; j++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[j]);
    if (dp)
      {
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[j]);
      }
    }

  //tell the extraction filter to execute
  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Select");

  // no need for the selector anymore.
  selectorProxy->Delete();

  return;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectPoints(
  vtkSMRenderModuleProxy* rmp, vtkSMProxy* inSel, vtkSMProxy* outSel)
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > sources;

  // unlike frustum, we don't do a pick first, so we just look for all
  // visible and selectable display objects instead.
  int numProps = rmp->GetDisplays()->GetNumberOfItems();
  for (int i = 0; i < numProps; i++)
    {
    vtkSMProxy* objProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::INPUT);
    vtkSMProxy* dispProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::DISPLAY);
    if (!dispProxy)
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
    // selectable
    displays.push_back(dispProxy);
    sources.push_back(objProxy);
    }
 
  //create the extraction filter on the server that will find the results
  vtkSMProxy* selectorProxy = 
    this->GetProxyManager()->NewProxy("selection_helpers", "ExtractSelection");
  if (!selectorProxy)
    {
    vtkErrorMacro("No selection proxy was created on the data server. "
      "Cannot select cells");
    return;
    }
  selectorProxy->SetConnectionID(rmp->GetConnectionID());
  selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);

  //tell the filter what type of extraction to do
  vtkSelection *sel = vtkSelection::New();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::POINTS
    );
  vtkDoubleArray* vals = vtkDoubleArray::New();
  vals->SetNumberOfComponents(3);
  vals->SetNumberOfTuples(this->NumPoints);
  for (int i = 0; i<this->NumPoints; i++)
    {
    vals->SetTupleValue(i, &this->Points[i*3]);
    }
  sel->SetSelectionList(vals);
  this->SendSelection(sel,inSel);
  vals->Delete();
  sel->Delete();
  
  vtkSMProxyProperty* inSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("InputSelection"));
  inSelectionProperty->AddProxy(inSel);

  //tell the filter to put its results where we can get them
  vtkSMProxyProperty* outSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("OutputSelection"));
  outSelectionProperty->AddProxy(outSel);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  //give the extraction filter the list of source algorithms and vtkProps
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = displays.size();
  for (unsigned int j = 0; j < numProxies; j++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[j]);
    if (dp)
      {
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[j]);
      }
    }

  //tell the extraction filter to execute
  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Select");

  // no need for the selector anymore.
  selectorProxy->Delete();

  return;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SelectThresholds(
  vtkSMRenderModuleProxy* rmp, vtkSMProxy* inSel, vtkSMProxy* outSel)
{  
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > sources;

  // unlike frustum, we don't do a pick first, so we just look for all
  // visible and selectable display objects instead.
  int numProps = rmp->GetDisplays()->GetNumberOfItems();
  for (int i = 0; i < numProps; i++)
    {
    vtkSMProxy* objProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::INPUT);
    vtkSMProxy* dispProxy = 
      rmp->GetProxyForDisplay(i, vtkSMRenderModuleProxy::DISPLAY);
    if (!dispProxy)
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
    // selectable
    displays.push_back(dispProxy);
    sources.push_back(objProxy);
    }

  //create the extraction filter on the server that will find the results
  vtkSMProxy* selectorProxy = 
    this->GetProxyManager()->NewProxy("selection_helpers", "ExtractSelection");
  if (!selectorProxy)
    {
    vtkErrorMacro("No selection proxy was created on the data server. "
      "Cannot select cells");
    return;
    }
  selectorProxy->SetConnectionID(rmp->GetConnectionID());
  selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);

  //tell the filter what type of extraction to do
  vtkSelection *sel = vtkSelection::New();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::THRESHOLDS
    );
  vtkDoubleArray* vals = vtkDoubleArray::New();
  vals->SetNumberOfComponents(1);
  vals->SetNumberOfTuples(this->NumThresholds*2);
  for (int i = 0; i<this->NumThresholds; i++)
    {
    vals->SetValue(i*2+0, this->Thresholds[i*2+0]);
    vals->SetValue(i*2+1, this->Thresholds[i*2+1]);
    }
  sel->SetSelectionList(vals);
  this->SendSelection(sel,inSel);
  vals->Delete();
  sel->Delete();
  
  vtkSMProxyProperty* inSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("InputSelection"));
  inSelectionProperty->AddProxy(inSel);

  //tell the filter to put its results where we can get them
  vtkSMProxyProperty* outSelectionProperty = 
    vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("OutputSelection"));
  outSelectionProperty->AddProxy(outSel);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  //give the extraction filter the list of source algorithms and vtkProps
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = displays.size();
  for (unsigned int j = 0; j < numProxies; j++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[j]);
    if (dp)
      {
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[j]);
      }
    }

  //tell the extraction filter to execute
  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Select");

  // no need for the selector anymore.
  selectorProxy->Delete();

  return;
}

//-----------------------------------------------------------------------------
//finds the client server ids of the algorithms that produced the props we selected on
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
    if (objProxy)
      {
      coll->AddItem(objProxy); 
      }
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
  os << indent << "RenderModule: " << this->RenderModule << endl;
  os << indent << "SelectionUpToDate: " << this->SelectionUpToDate << endl;
  os << indent << "ScreenRectangle: " 
    << this->ScreenRectangle[0] << ","
    << this->ScreenRectangle[1] << " to "
    << this->ScreenRectangle[2] << ","
    << this->ScreenRectangle[3] << endl;
  os << indent << "Mode: ";
  switch (this->Mode)
    {
  case SURFACE:
    os << "SURFACE";
    break;

  case FRUSTUM:
    os << "FRUSTUM";
    break;

  case IDS:
    os << "IDS";
    break;

  case POINTS:
    os << "POINTS";
    break;

  case THRESHOLDS:
    os << "THRESHOLDS";
    break;

  default:
    os << "(Unknown)";
    }
  os << endl;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetNumIds(int num)
{
  if (this->Ids != NULL)
    {
    delete[] this->Ids;
    }
  this->Ids = new int[num];
  this->NumIds = num;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetIds(int i, vtkIdType *vals)
{
  if (this->Ids == NULL)
    {
    return;
    }
  memcpy(&this->Ids[i], vals, sizeof(vtkIdType));
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetNumPoints(int num)
{
  if (this->Points != NULL)
    {
    delete[] this->Points;
    }
  this->Points = new double[num*3];
  this->NumPoints = num;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetPoints(int i, double *vals)
{
  if (this->Points == NULL)
    {
    return;
    }
  memcpy(&this->Points[i*3], vals, sizeof(double)*3);
}
//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetNumThresholds(int num)
{
  if (this->Thresholds != NULL)
    {
    delete[] this->Thresholds;
    }
  this->Thresholds = new double[num*2];
  this->NumThresholds = num;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionProxy::SetThresholds(int i, double *vals)
{
  if (this->Thresholds == NULL)
    {
    return;
    }
  memcpy(&this->Thresholds[i*2], vals, sizeof(double)*2);
}


