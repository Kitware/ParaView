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
#include "vtkSMQuadViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVQuadRenderView.h"
#include "vtkPVQuadViewInformation.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVSession.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

#include <assert.h>

namespace
{
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
}

vtkStandardNewMacro(vtkSMQuadViewProxy);
//----------------------------------------------------------------------------
vtkSMQuadViewProxy::vtkSMQuadViewProxy()
{
  this->WidgetLinker = vtkSMProxyLink::New();
  this->WidgetLinker->PropagateUpdateVTKObjectsOff();
}

//----------------------------------------------------------------------------
vtkSMQuadViewProxy::~vtkSMQuadViewProxy()
{
  this->WidgetLinker->Delete();
  this->WidgetLinker = NULL;
}

//----------------------------------------------------------------------------
int vtkSMQuadViewProxy::CreateSubProxiesAndProperties(vtkSMSessionProxyManager* pm,
  vtkPVXMLElement *elm)
{
  int result = this->Superclass::CreateSubProxiesAndProperties(pm, elm);

  // Create link across proxies
  // Let's add our widgets sub-proxy to our hidden representation property
  vtkSMProxy* widget = NULL;
  widget = this->GetSubProxy("WidgetTopLeft");
  vtkSMPropertyHelper(widget, "Enabled").Set(1);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::INPUT);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::OUTPUT);

  widget = this->GetSubProxy("WidgetTopRight");
  vtkSMPropertyHelper(widget, "Enabled").Set(1);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::INPUT);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::OUTPUT);

  widget = this->GetSubProxy("WidgetBottomLeft");
  vtkSMPropertyHelper(widget, "Enabled").Set(1);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::INPUT);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::OUTPUT);

  widget = this->GetSubProxy("WidgetBottomRight");
  vtkSMPropertyHelper(widget, "Enabled").Set(1);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::INPUT);
  this->WidgetLinker->AddLinkedProxy(widget, vtkSMProxyLink::OUTPUT);

  // Bind the center
  this->WidgetLinker->AddLinkedProxy(this->GetSubProxy("SliceOrigin"), vtkSMProxyLink::INPUT);
  this->WidgetLinker->AddLinkedProxy(this->GetSubProxy("SliceOrigin"), vtkSMProxyLink::OUTPUT);

  return result;
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if (this->Location == 0 || !this->ObjectsCreated)
    {
    return;
    }

  vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(
    this->GetClientSideObject());
  for (int cc=0; cc < 3; cc++)
    {
    vtkNew<vtkRenderHelper> helper;
    helper->Proxy = this;
    quadView->GetOrthoRenderView(cc)->GetInteractor()->SetPVRenderView(
      helper.GetPointer());
    }

  // Register widgets
  vtkSMPropertyHelper(this, "TopLeftRepresentations").Add(this->GetSubProxy("WidgetTopLeft"));
  vtkSMPropertyHelper(this, "TopRightRepresentations").Add(this->GetSubProxy("WidgetTopRight"));
  vtkSMPropertyHelper(this, "BottomLeftRepresentations").Add(this->GetSubProxy("WidgetBottomLeft"));
  vtkSMPropertyHelper(this, "HiddenRepresentations").Add(this->GetSubProxy("WidgetBottomRight"));

  // Register origin listener
  vtkSMPropertyHelper(this, "SliceOriginSource").Add(this->GetSubProxy("SliceOrigin"));
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMQuadViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "QuadViewCompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "QuadViewCompositeMultiSliceRepresentation"));
    return repr;
    }

  // Currently only images can be shown
  vtkErrorMacro("This view only supports Multi-Slice representation.");
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkSMQuadViewProxy::IsSelectVisiblePointsAvailable()
{
  // The original dataset and the slice don't share the same points
  return "Quad View do not allow point selection";
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMQuadViewProxy::CaptureWindowInternal(int magnification)
{
  vtkPVQuadRenderView* quadView =
      vtkPVQuadRenderView::SafeDownCast(this->GetClientSideObject());

  // Global var used to loop over
  vtkRenderWindow* allWindows[4] = {
    quadView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_LEFT),
    quadView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_RIGHT),
    quadView->GetOrthoViewWindow(vtkPVQuadRenderView::BOTTOM_LEFT),
    quadView->GetRenderWindow()
  };

#if !defined(__APPLE__)
  vtkPVRenderView* allView[4] = {
    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_LEFT),
    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_RIGHT),
    quadView->GetOrthoRenderView(vtkPVQuadRenderView::BOTTOM_LEFT),
    quadView
  };
#endif

  // Combined image vars
  vtkImageData* finalImage = vtkImageData::New();

  // Image generation
  vtkImageData* currentWindowImage = NULL;
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();

  for(int i=0; i < 4; ++i)
    {
#if !defined(__APPLE__)
    vtkPVRenderView* view = allView[i];
    vtkRenderWindow* window = allWindows[i];
    int prevOffscreen = window->GetOffScreenRendering();
    bool use_offscreen = view->GetUseOffscreenRendering() ||
                         view->GetUseOffscreenRenderingForScreenshots();
    window->SetOffScreenRendering(use_offscreen? 1: 0);
#endif

    allWindows[i]->SwapBuffersOff();
    this->CaptureWindowInternalRender();

    w2i->SetInput(allWindows[i]);
    // BUG #8715: We go through this indirection since the active connection needs
    // to be set during update since it may request re-renders if magnification >1.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << w2i.GetPointer() << "Update"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, vtkProcessModule::CLIENT);

    allWindows[i]->SwapBuffersOn();

#if !defined(__APPLE__)
    window->SetOffScreenRendering(prevOffscreen);
#endif

    // Update capture information and write data into it.
    currentWindowImage = w2i->GetOutput();

    // Initialize big picture if needed
    if(i==0)
      {
      // Need to initialize the size of the output data
      int dimension[3];
      currentWindowImage->GetDimensions(dimension);
      dimension[0] *= 2;
      dimension[1] *= 2;
      finalImage->SetDimensions(dimension);
      finalImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
      }

    // There is a bug where the cloned views do not correctly
    // track their own view position (and therefore extent),
    // so for now we'll manually adjust their extent information.
    this->UpdateInternalViewExtent(currentWindowImage, i%2, i/2);

    // Merge view into fullImage
    vtkSMAnimationSceneImageWriter::Merge(finalImage, currentWindowImage);
    allWindows[i]->Frame();
    }

  return finalImage;
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::UpdateInternalViewExtent(vtkImageData * image,
                                                  int columnIndex, int rowIndex)
{
  int extent[6];
  int dimensions[3];
  image->GetDimensions(dimensions);
  image->GetExtent(extent);
  extent[0] = columnIndex*dimensions[0];
  extent[1] = extent[0]+dimensions[0]-1;
  extent[2] = rowIndex*dimensions[1];
  extent[3] = extent[2]+dimensions[1]-1;
  extent[4] = extent[5] = 0;
  image->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::PostRender(bool interactive)
{
  this->Superclass::PostRender(interactive);
  if(!interactive)
    {
    // Not interacting anymore, let's gather informations
    vtkNew<vtkPVQuadViewInformation> info;
    this->GatherInformation(info.GetPointer(), vtkPVSession::DATA_SERVER);
    vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(
      this->GetClientSideObject());

    // Update Informations
    quadView->SetXAxisLabel(info->GetXLabel());
    quadView->SetYAxisLabel(info->GetYLabel());
    quadView->SetZAxisLabel(info->GetZLabel());
    quadView->SetScalarLabel(info->GetScalarLabel());
    quadView->SetScalarValue(info->GetValues()[3]);
    }
}
