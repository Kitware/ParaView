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
#include "vtkSMRenderViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVLastSelectionInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSession.h"
#include "vtkTransform.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

#include <vtkstd/map>

namespace
{
  bool vtkIsImageEmpty(vtkImageData* image)
    {
    vtkDataArray* scalars = image->GetPointData()->GetScalars();
    for (int comp=0; comp < scalars->GetNumberOfComponents(); comp++)
      {
      double range[2];
      scalars->GetRange(range, comp);
      if (range[0] != 0.0 || range[1] != 0.0)
        {
        return false;
        }
      }
    return true;
    }

  class vtkRenderHelper : public vtkPVRenderViewProxy
  {
public:
  static vtkRenderHelper* New();
  vtkTypeMacro(vtkRenderHelper, vtkPVRenderViewProxy);

  virtual void EventuallyRender()
    {
    this->Proxy->StillRender();
    }
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }
  virtual void Render()
    {
    this->Proxy->InteractiveRender();
    }
  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive()
    {
    return this->Proxy->LastRenderWasInteractive();
    }

  vtkWeakPointer<vtkSMRenderViewProxy> Proxy;
  };
  vtkStandardNewMacro(vtkRenderHelper);
};

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
{
  this->IsSelectionCached = false;
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::LastRenderWasInteractive()
{
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetUsedLODForLastRender() : false;
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::IsSelectionAvailable()
{
  const char* msg = this->IsSelectVisibleCellsAvailable();
  if (msg)
    {
    //vtkErrorMacro(<< msg);
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisibleCellsAvailable()
{
  vtkSMSession* session = this->GetSession();

  if (session->GetIsAutoMPI())
    {
    return "Cannot support selection in auto-mpi mode";
    }
  if (session->GetController(vtkPVSession::DATA_SERVER_ROOT) !=
    session->GetController(vtkPVSession::RENDER_SERVER_ROOT))
    {
    // when the two controller are different, we have a separate render-server
    // and data-server session.
    return "Cannot support selection in render-server mode";
    }

  vtkPVServerInformation* server_info = session->GetServerInformation();
  if (server_info && server_info->GetNumberOfMachines() > 0)
    {
    return "Cannot support selection in CAVE mode.";
    }

  //check if we don't have enough color depth to do color buffer selection
  //if we don't then disallow selection
  int rgba[4];
  vtkRenderWindow *rwin = this->GetRenderWindow();
  if (!rwin)
    {
    return "No render window available";
    }

  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    return "Selection not supported due to insufficient color depth.";
    }

  return NULL;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisiblePointsAvailable()
{
  return this->IsSelectVisibleCellsAvailable();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PostRender(bool interactive)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  this->SynchronizeCameraProperties();
  this->Superclass::PostRender(interactive);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SynchronizeCameraProperties()
{
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  vtkSMPropertyIterator* iter = cameraProxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *cur_property = iter->GetProperty();
    vtkSMProperty *info_property = cur_property->GetInformationProperty();
    if (!info_property)
      {
      continue;
      }
    cur_property->Copy(info_property);
    //cur_property->UpdateLastPushedValues();
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMRenderViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetRenderWindow() : NULL;
}

//----------------------------------------------------------------------------
vtkRenderer* vtkSMRenderViewProxy::GetRenderer()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetRenderer() : NULL;
}

//----------------------------------------------------------------------------
vtkCamera* vtkSMRenderViewProxy::GetActiveCamera()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetActiveCamera() : NULL;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor* vtkSMRenderViewProxy::GetInteractor()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetInteractor() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if(this->Location == 0)
    {
    return;
    }

  if (!this->ObjectsCreated)
    {
    return;
    }


  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());

#if 0
  vtkCamera* camera = vtkCamera::SafeDownCast(this
                                              ->GetSubProxy( "ActiveCamera" )
                                              ->GetClientSideObject() );
  rv->SetActiveCamera( camera );
#else
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "SetActiveCamera"
         << VTKOBJECT(cameraProxy)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
#endif

  if (rv->GetInteractor())
    {
    vtkRenderHelper* helper = vtkRenderHelper::New();
    helper->Proxy = this;
    rv->GetInteractor()->SetPVRenderView(helper);
    helper->Delete();
    }

  vtkEventForwarderCommand* forwarder = vtkEventForwarderCommand::New();
  forwarder->SetTarget(this);
  rv->AddObserver(vtkCommand::SelectionChangedEvent, forwarder);
  rv->AddObserver(vtkCommand::ResetCameraEvent, forwarder);
  forwarder->Delete();

  // We'll do this for now. But we need to not do this here. I am leaning
  // towards not making stereo a command line option as mentioned by a very
  // not-too-pleased user on the mailing list a while ago.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* pvoptions = pm->GetOptions();
  if (pvoptions->GetUseStereoRendering())
    {
    vtkSMPropertyHelper(this, "StereoCapableWindow").Set(1);
    vtkSMPropertyHelper(this, "StereoRender").Set(1);
    vtkSMEnumerationDomain* domain = vtkSMEnumerationDomain::SafeDownCast(
      this->GetProperty("StereoType")->GetDomain("enum"));
    if (domain && domain->HasEntryText(pvoptions->GetStereoType()))
      {
      vtkSMPropertyHelper(this, "StereoType").Set(
        domain->GetEntryValueForText(pvoptions->GetStereoType()));
      }
    }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = source->GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "UnstructuredGridRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UnstructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "UniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UniformGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "GeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "GeometryRepresentation"));
    }

  vtkPVXMLElement* hints = source->GetHints();
  if (hints)
    {
    // If the source has an hint as follows, then it's a text producer and must
    // be is display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>

    unsigned int numElems = hints->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char *childName = child->GetName();
      if (childName &&
        strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) &&
        index == opport &&
        child->GetAttribute("type") &&
        strcmp(child->GetAttribute("type"), "text") == 0)
        {
        return vtkSMRepresentationProxy::SafeDownCast(
          pxm->NewProxy("representations", "TextSourceRepresentation"));
        }
      else if(childName && strcmp(childName, "DefaultRepresentations") == 0)
        {
        unsigned int defaultRepCount = child->GetNumberOfNestedElements();
        for(unsigned int i = 0; i < defaultRepCount; i++)
          {
          vtkPVXMLElement *defaultRep = child->GetNestedElement(i);
          const char *representation = defaultRep->GetAttribute("representation");

          return vtkSMRepresentationProxy::SafeDownCast(
            pxm->NewProxy("representations", representation));
          }
        }

      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ZoomTo(vtkSMProxy* representation)
{
  vtkSMPropertyHelper helper(representation, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  int port =helper.GetOutputPort();
  if (!input)
    {
    return;
    }

  vtkPVDataInformation* info = input->GetDataInformation(port);
  double bounds[6];
  info->GetBounds(bounds);
  if (!vtkMath::AreBoundsInitialized(bounds))
    {
    return;
    }

  if (representation->GetProperty("Position") &&
    representation->GetProperty("Orientation") &&
    representation->GetProperty("Scale"))
    {
    double position[3], rotation[3], scale[3];
    vtkSMPropertyHelper(representation, "Position").Get(position, 3);
    vtkSMPropertyHelper(representation, "Orientation").Get(rotation, 3);
    vtkSMPropertyHelper(representation, "Scale").Get(scale, 3);

    if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 ||
      position[0] != 0.0 || position[1] != 0.0 || position[2] != 0.0 ||
      rotation[0] != 0.0 || rotation[1] != 0.0 || rotation[2] != 0.0)
      {
      vtkTransform* transform = vtkTransform::New();
      transform->Translate(position);
      transform->RotateZ(rotation[2]);
      transform->RotateX(rotation[0]);
      transform->RotateY(rotation[1]);
      transform->Scale(scale);

      int i, j, k;
      double origX[3], x[3];
      vtkBoundingBox bbox;
      for (i = 0; i < 2; i++)
        {
        origX[0] = bounds[i];
        for (j = 0; j < 2; j++)
          {
          origX[1] = bounds[2 + j];
          for (k = 0; k < 2; k++)
            {
            origX[2] = bounds[4 + k];
            transform->TransformPoint(origX, x);
            bbox.AddPoint(x);
            }
          }
        }
      bbox.GetBounds(bounds);
      transform->Delete();
      }
    }
  this->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(double bounds[6])
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "ResetCamera"
         << vtkClientServerStream::InsertArray(bounds, 6)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (this->IsSelectionCached)
    {
    this->IsSelectionCached = false;
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << VTKOBJECT(this)
            << "InvalidateCachedSelection"
            << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  // skip modified properties on camera subproxy.
  if (modifiedProxy != this->GetSubProxy("ActiveCamera"))
    {
    this->Superclass::MarkDirty(modifiedProxy);
    }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::Pick(int x, int y)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the
  //   visible props from all representations.
  vtkSMRepresentationProxy* repr = NULL;
  vtkCollection* reprs = vtkCollection::New();
  vtkCollection* sources = vtkCollection::New();
  int region[4] = {x,y,x,y};
  if (this->SelectSurfaceCells(region, reprs, sources, false))
    {
    if (reprs->GetNumberOfItems() > 0)
      {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
      }
    }
  reprs->Delete();
  sources->Delete();
  return repr;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfaceCells(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }

  this->IsSelectionCached = true;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "SelectCells"
         << region[0] << region[1] << region[2] << region[3]
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  return this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfacePoints(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }

  this->IsSelectionCached = true;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "SelectPoints"
         << region[0] << region[1] << region[2] << region[3]
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  return this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources);
}

namespace
{
  //-----------------------------------------------------------------------------
  static void vtkShrinkSelection(vtkSelection* sel)
    {
    vtkstd::map<int, int> pixelCounts;
    unsigned int numNodes = sel->GetNumberOfNodes();
    int choosen = -1;
    int maxPixels = -1;
    for (unsigned int cc=0; cc < numNodes; cc++)
      {
      vtkSelectionNode* node = sel->GetNode(cc);
      vtkInformation* properties = node->GetProperties();
      if (properties->Has(vtkSelectionNode::PIXEL_COUNT()) &&
        properties->Has(vtkSelectionNode::SOURCE_ID()))
        {
        int numPixels = properties->Get(vtkSelectionNode::PIXEL_COUNT());
        int source_id = properties->Get(vtkSelectionNode::SOURCE_ID());
        pixelCounts[source_id] += numPixels;
        if (pixelCounts[source_id] > maxPixels)
          {
          maxPixels = numPixels;
          choosen = source_id;
          }
        }
      }

    vtkstd::vector<vtkSmartPointer<vtkSelectionNode> > choosenNodes;
    if (choosen != -1)
      {
      for (unsigned int cc=0; cc < numNodes; cc++)
        {
        vtkSelectionNode* node = sel->GetNode(cc);
        vtkInformation* properties = node->GetProperties();
        if (properties->Has(vtkSelectionNode::SOURCE_ID()) &&
          properties->Get(vtkSelectionNode::SOURCE_ID()) == choosen)
          {
          choosenNodes.push_back(node);
          }
        }
      }
    sel->RemoveAllNodes();
    for (unsigned int cc=0; cc <choosenNodes.size(); cc++)
      {
      sel->AddNode(choosenNodes[cc]);
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::FetchLastSelection(
  bool multiple_selections,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources)
{
  if (selectionSources && selectedRepresentations)
    {
    vtkSmartPointer<vtkPVLastSelectionInformation> info =
      vtkSmartPointer<vtkPVLastSelectionInformation>::New();

    this->GetSession()->GatherInformation(
      vtkPVSession::DATA_SERVER, info, this->GetGlobalID());

    vtkSelection* selection = info->GetSelection();
    if (!multiple_selections)
      {
      // only pass through selection over a single representation.
      vtkShrinkSelection(selection);
      }
    vtkSMSelectionHelper::NewSelectionSourcesFromSelection(
      selection, this, selectionSources, selectedRepresentations);
    return (selectionSources->GetNumberOfItems() > 0);
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumCells(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  return this->SelectFrustumInternal(region, selectedRepresentations,
    selectionSources, multiple_selections, vtkSelectionNode::CELL);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumPoints(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  return this->SelectFrustumInternal(region, selectedRepresentations,
    selectionSources, multiple_selections, vtkSelectionNode::POINT);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumInternal(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections,
  int fieldAssociation)
{
  // Simply stealing old code for now. This code have many coding style
  // violations and seems too long for what it does. At some point we'll check
  // it out.

  int displayRectangle[4] = {region[0], region[1], region[2], region[3]};
  if (displayRectangle[0] == displayRectangle[2])
    {
    displayRectangle[2] += 1;
    }
  if (displayRectangle[1] == displayRectangle[3])
    {
    displayRectangle[3] += 1;
    }

  // 1) Create frustum selection
  //convert screen rectangle to world frustum
  vtkRenderer *renderer = this->GetRenderer();
  double frustum[32];
  int index=0;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);

  vtkSMProxy* selectionSource = this->GetProxyManager()->NewProxy("sources",
    "FrustumSelectionSource");
  vtkSMPropertyHelper(selectionSource, "FieldType").Set(fieldAssociation);
  vtkSMPropertyHelper(selectionSource, "Frustum").Set(frustum, 32);
  selectionSource->UpdateVTKObjects();

  // 2) Figure out which representation is "selected".
  vtkExtractSelectedFrustum* extractor =
    vtkExtractSelectedFrustum::New();
  extractor->CreateFrustum(frustum);

  // Now we just use the first selected representation,
  // until we have other mechanisms to select one.
  vtkSMPropertyHelper reprsHelper(this, "Representations");

  for (unsigned int cc=0;  cc < reprsHelper.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(reprsHelper.GetAsProxy(cc));
    if (!repr || vtkSMPropertyHelper(repr, "Visibility", true).GetAsInt() == 0)
      {
      continue;
      }
    if (vtkSMPropertyHelper(repr, "Pickable", true).GetAsInt() == 0)
      {
      // skip non-pickable representations.
      continue;
      }
    vtkPVDataInformation* datainfo = repr->GetRepresentedDataInformation();
    if (!datainfo)
      {
      continue;
      }

    double bounds[6];
    datainfo->GetBounds(bounds);

    if (extractor->OverallBoundsTest(bounds))
      {
      selectionSources->AddItem(selectionSource);
      selectedRepresentations->AddItem(repr);
      if (!multiple_selections)
        {
        break;
        }
      }
    }

  extractor->Delete();
  selectionSource->Delete();
  return true;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CaptureWindowInternalRender()
{
  vtkPVRenderView* view =
    vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  if (view->GetUseInteractiveRenderingForSceenshots())
    {
    this->InteractiveRender();
    }
  else
    {
    this->StillRender();
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMRenderViewProxy::CaptureWindowInternal(int magnification)
{
#if !defined(__APPLE__)
  vtkPVRenderView* view =
    vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
#endif

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  vtkRenderWindow* window = this->GetRenderWindow();
  int prevOffscreen = window->GetOffScreenRendering();
  bool use_offscreen = view->GetUseOffscreenRendering() ||
    view->GetUseOffscreenRenderingForScreenshots();
  window->SetOffScreenRendering(use_offscreen? 1: 0);
#endif

  this->GetRenderWindow()->SwapBuffersOff();

  this->CaptureWindowInternalRender();

  vtkSmartPointer<vtkWindowToImageFilter> w2i =
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();

  // BUG #8715: We go through this indirection since the active connection needs
  // to be set during update since it may request re-renders if magnification >1.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << w2i.GetPointer() << "Update"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkProcessModule::CLIENT);

  this->GetRenderWindow()->SwapBuffersOn();

#if !defined(__APPLE__)
  window->SetOffScreenRendering(prevOffscreen);

  if (view->GetUseOffscreenRenderingForScreenshots() &&
    vtkIsImageEmpty(w2i->GetOutput()))
    {
    // ensure that some image was capture. Due to buggy offscreen rendering
    // support on some drivers, we may end up with black images, in which case
    // we force on-screen rendering.
    if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() == 1)
      {
      vtkWarningMacro(
        "Disabling offscreen rendering since empty image was detected.");
      view->SetUseOffscreenRenderingForScreenshots(false);
      return this->CaptureWindowInternal(magnification);
      }
    }
#endif

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  this->GetRenderWindow()->Frame();
  return capture;
}

//----------------------------------------------------------------------------
double vtkSMRenderViewProxy::GetZBufferValue(int x, int y)
{
  this->Session->Activate();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  double result = rv? rv->GetZbufferDataAtPoint(x, y) : 1.0;
  this->Session->DeActivate();

  return result;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
