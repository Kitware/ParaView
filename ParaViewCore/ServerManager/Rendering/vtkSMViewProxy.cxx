/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkImageData.h"
#include "vtkImageTransparencyFilter.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVView.h"
#include "vtkPVXMLElement.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkStereoCompositor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

#include <assert.h>

namespace vtkSMViewProxyNS
{
const char* GetRepresentationNameFromHints(const char* viewType, vtkPVXMLElement* hints, int port)
{
  if (!hints)
  {
    return NULL;
  }

  for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child == NULL || child->GetName() == NULL)
    {
      continue;
    }

    // LEGACY: support DefaultRepresentations hint.
    // <Hints>
    //    <DefaultRepresentations representation="Foo" />
    // </Hints>
    if (strcmp(child->GetName(), "DefaultRepresentations") == 0)
    {
      return child->GetAttribute("representation");
    }

    // <Hints>
    //    <Representation port="outputPort" view="ViewName" type="ReprName" />
    // </Hints>
    else if (strcmp(child->GetName(), "Representation") == 0 &&
      // has an attribute "view" that matches the viewType.
      child->GetAttribute("view") && strcmp(child->GetAttribute("view"), viewType) == 0 &&
      child->GetAttribute("type") != NULL)
    {
      // if port is present, it must match "port".
      int xmlPort;
      if (child->GetScalarAttribute("port", &xmlPort) == 0 || xmlPort == port)
      {
        return child->GetAttribute("type");
      }
    }
  }
  return NULL;
}

/**
 * Extends vtkWindowToImageFilter to call
 * `vtkSMViewProxy::RenderForImageCapture()` when the filter wants to request a
 * render.
 */
class WindowToImageFilter : public vtkWindowToImageFilter
{
public:
  static WindowToImageFilter* New();
  vtkTypeMacro(WindowToImageFilter, vtkWindowToImageFilter);

  void SetParent(vtkSMViewProxy* view) { this->Parent = view; }

protected:
  WindowToImageFilter() {}
  ~WindowToImageFilter() override {}

  void Render() override
  {
    if (this->Parent)
    {
      this->Parent->RenderForImageCapture();
    }
  }

  vtkWeakPointer<vtkSMViewProxy> Parent;

private:
  WindowToImageFilter(const WindowToImageFilter&) = delete;
  void operator=(const WindowToImageFilter&) = delete;
};
vtkStandardNewMacro(WindowToImageFilter);

/**
 * Helper to help with image capture to handle all sorts of configurations
 * including transparent background and stereo.
 */

class CaptureHelper
{
  vtkSMViewProxy* Self = nullptr;

public:
  CaptureHelper(vtkSMViewProxy* self)
    : Self(self)
  {
  }
  ~CaptureHelper() = default;

  vtkSmartPointer<vtkImageData> StereoCapture(int magX, int magY)
  {
    auto self = this->Self;
    vtkRenderWindow* window = self->GetRenderWindow();
    if (!window)
    {
      return this->TranslucentCapture(magX, magY);
    }

    const bool stereo_render = vtkSMPropertyHelper(self, "StereoRender", true).GetAsInt() == 1;
    const int stereo_type = vtkSMPropertyHelper(self, "StereoType", true).GetAsInt();

    // determine if we're capture a stereo image that needs two passes.
    const bool two_pass_stereo =
      stereo_render &&
      (stereo_type == VTK_STEREO_RED_BLUE || stereo_type == VTK_STEREO_ANAGLYPH ||
        stereo_type == VTK_STEREO_INTERLACED || stereo_type == VTK_STEREO_DRESDEN ||
        stereo_type == VTK_STEREO_CHECKERBOARD ||
        stereo_type == VTK_STEREO_SPLITVIEWPORT_HORIZONTAL);

    const bool one_pass_stereo =
      stereo_render && (stereo_type == VTK_STEREO_LEFT || stereo_type == VTK_STEREO_RIGHT);

    if (!one_pass_stereo && !two_pass_stereo && stereo_render)
    {
      // The render window is using crystal eyes, fake or emulate modes.
      // All of these modes don't impact on how images are saved. So disable stereo.
      vtkSMPropertyHelper(self, "StereoRender").Set(0);
    }

    vtkSmartPointer<vtkImageData> img;
    if (two_pass_stereo)
    {
      // two_pass_stereo doesn't support transparent background since
      // `vtkStereoCompositor` currently only works with RGB images.
      vtkSmartPointer<vtkImageData> images[2];
      vtkSMPropertyHelper(self, "StereoType").Set(VTK_STEREO_LEFT);
      self->UpdateVTKObjects();
      images[0] = this->Capture(magX, magY);

      vtkSMPropertyHelper(self, "StereoType").Set(VTK_STEREO_RIGHT);
      self->UpdateVTKObjects();
      images[1] = this->Capture(magX, magY);

      this->StereoCompose(stereo_type, images[0], images[1]);
      img = images[0];
    }
    else
    {
      img = this->TranslucentCapture(magX, magY);
    }

    vtkSMPropertyHelper(self, "StereoRender", true).Set(stereo_render ? 1 : 0);
    vtkSMPropertyHelper(self, "StereoType", true).Set(stereo_type);
    self->UpdateVTKObjects();
    return img;
  }

private:
  vtkSmartPointer<vtkImageData> TranslucentCapture(int magX, int magY)
  {
    auto self = this->Self;
    vtkRenderWindow* window = self->GetRenderWindow();
    if (window && vtkSMViewProxy::GetTransparentBackground())
    {
      if (auto renderer = this->FindBackgroundRenderer(window))
      {
        std::unique_ptr<RendererSaverRAII> rsaver(new RendererSaverRAII(renderer));
        renderer->SetGradientBackground(false);
        renderer->SetTexturedBackground(false);
        renderer->SetBackground(1.0, 1.0, 1.0);
        auto whiteImage = this->Capture(magX, magY);
        renderer->SetBackground(0, 0, 0);
        auto blackImage = this->Capture(magX, magY);
        rsaver.reset();

        vtkNew<vtkImageTransparencyFilter> tfilter;
        tfilter->AddInputData(whiteImage);
        tfilter->AddInputData(blackImage);
        tfilter->Update();

        vtkSmartPointer<vtkImageData> result;
        result = tfilter->GetOutput();
        return result;
      }
    }

    return this->Capture(magX, magY);
  }

  vtkSmartPointer<vtkImageData> Capture(int magX, int magY)
  {
    return vtkSmartPointer<vtkImageData>::Take(this->Self->CaptureWindowSingle(magX, magY));
  }

  vtkRenderer* FindBackgroundRenderer(vtkRenderWindow* window) const
  {
    vtkCollectionSimpleIterator cookie;
    auto renderers = window->GetRenderers();
    renderers->InitTraversal(cookie);
    while (auto renderer = renderers->GetNextRenderer(cookie))
    {
      if (renderer->GetErase())
      {
        // Found a background-writing renderer.
        return renderer;
      }
    }

    return nullptr;
  }

  class RendererSaverRAII
  {
    vtkRenderer* Renderer;
    const bool Gradient;
    const bool Textured;
    const double Background[3];

  public:
    RendererSaverRAII(vtkRenderer* ren)
      : Renderer(ren)
      , Gradient(ren->GetGradientBackground())
      , Textured(ren->GetTexturedBackground())
      , Background{ ren->GetBackground()[0], ren->GetBackground()[1], ren->GetBackground()[2] }
    {
    }

    ~RendererSaverRAII()
    {
      this->Renderer->SetGradientBackground(this->Gradient);
      this->Renderer->SetTexturedBackground(this->Textured);
      this->Renderer->SetBackground(const_cast<double*>(this->Background));
    }
  };

  bool StereoCompose(int type, vtkImageData* leftNResult, vtkImageData* right)
  {
    vtkRenderWindow* renWin = this->Self->GetRenderWindow();
    assert(renWin);

    auto lbuffer = vtkUnsignedCharArray::SafeDownCast(leftNResult->GetPointData()->GetScalars());
    auto rbuffer = vtkUnsignedCharArray::SafeDownCast(right->GetPointData()->GetScalars());
    if (lbuffer == nullptr || rbuffer == nullptr)
    {
      return false;
    }

    const int size[2] = { leftNResult->GetDimensions()[0], leftNResult->GetDimensions()[1] };
    vtkNew<vtkStereoCompositor> compositor;
    switch (type)
    {
      case VTK_STEREO_RED_BLUE:
        return compositor->RedBlue(lbuffer, rbuffer);

      case VTK_STEREO_ANAGLYPH:
        return compositor->Anaglyph(
          lbuffer, rbuffer, renWin->GetAnaglyphColorSaturation(), renWin->GetAnaglyphColorMask());

      case VTK_STEREO_INTERLACED:
        return compositor->Interlaced(lbuffer, rbuffer, size);

      case VTK_STEREO_DRESDEN:
        return compositor->Dresden(lbuffer, rbuffer, size);

      case VTK_STEREO_CHECKERBOARD:
        return compositor->Checkerboard(lbuffer, rbuffer, size);

      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        return compositor->SplitViewportHorizontal(lbuffer, rbuffer, size);
    }

    return false;
  }

private:
  CaptureHelper(const CaptureHelper&) = delete;
  void operator=(const CaptureHelper&) = delete;
};

} // end of vtkSMViewProxyNS

bool vtkSMViewProxy::TransparentBackground = false;

vtkStandardNewMacro(vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->SetLocation(vtkProcessModule::CLIENT_AND_SERVERS);
  this->DefaultRepresentationName = 0;
  this->Enable = true;
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy()
{
  this->SetDefaultRepresentationName(0);
}

//----------------------------------------------------------------------------
vtkView* vtkSMViewProxy::GetClientSideView()
{
  if (this->ObjectsCreated)
  {
    return vtkView::SafeDownCast(this->GetClientSideObject());
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go further...
  if (this->Location == 0)
  {
    return;
  }

  if (auto object = vtkObject::SafeDownCast(this->GetClientSideObject()))
  {
    object->AddObserver(vtkPVView::ViewTimeChangedEvent, this, &vtkSMViewProxy::ViewTimeChanged);
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ViewTimeChanged()
{
  vtkSMPropertyHelper helper1(this, "Representations");
  for (unsigned int cc = 0; cc < helper1.GetNumberOfElements(); cc++)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(helper1.GetAsProxy(cc));
    if (repr)
    {
      repr->ViewTimeChanged();
    }
  }

  vtkSMPropertyHelper helper2(this, "HiddenRepresentations", true);
  for (unsigned int cc = 0; cc < helper2.GetNumberOfElements(); cc++)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(helper2.GetAsProxy(cc));
    if (repr)
    {
      repr->ViewTimeChanged();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  // bug 0013947
  // on Mac OSX don't render into invalid drawable, all subsequent
  // OpenGL calls fail with invalid framebuffer operation.
  if (this->IsContextReadyForRendering() == false)
  {
    return;
  }

  int interactive = 0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  this->GetSession()->PrepareProgress();
  // We call update separately from the render. This is done so that we don't
  // get any synchronization issues with GUI responding to the data-updated
  // event by making some data information requests(for example). If those
  // happen while StillRender/InteractiveRender is being executed on the server
  // side then we get deadlocks.
  this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive == 1);

  if (this->ObjectsCreated)
  {
    auto window = this->GetRenderWindow();
    vtkClientServerStream stream;
    if (window)
    {
      int tileScale[2];
      window->GetTileScale(tileScale);
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetTileScale" << tileScale[0]
             << tileScale[1] << vtkClientServerStream::End;

      double tileViewport[4];
      window->GetTileViewport(tileViewport);
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetTileViewport"
             << tileViewport[0] << tileViewport[1] << tileViewport[2] << tileViewport[3]
             << vtkClientServerStream::End;
    }
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "StillRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
  }

  this->PostRender(interactive == 1);
  this->GetSession()->CleanupPendingProgress();
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InteractiveRender()
{
  int interactive = 1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  this->GetSession()->PrepareProgress();

  // Ensure that data is up-to-date. This class keeps track of whether an
  // update is actually needed. If not, the update is essentially a no-op, so
  // it is fast.
  this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive == 1);

  if (this->ObjectsCreated)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "InteractiveRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
  }

  this->PostRender(interactive == 1);
  this->GetSession()->CleanupPendingProgress();
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::Update()
{
  if (this->ObjectsCreated && this->NeedsUpdate)
  {
    vtkClientServerStream stream;

    // To avoid race conditions in multi-client modes, we are taking a peculiar
    // approach. Any ivar that affect parallel communication are overridden
    // using the client-side values in the same ExecuteStream() call. That
    // ensures that two clients cannot enter race condition. This results in minor
    // increase in the size of the messages sent, but overall the benefits are
    // greater.
    vtkPVView* pvview = vtkPVView::SafeDownCast(this->GetClientSideObject());
    if (pvview)
    {
      int use_cache = pvview->GetUseCache() ? 1 : 0;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetUseCache" << use_cache
             << vtkClientServerStream::End;
    }
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "Update"
           << vtkClientServerStream::End;
    this->GetSession()->PrepareProgress();
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();

    unsigned int numProducers = this->GetNumberOfProducers();
    for (unsigned int i = 0; i < numProducers; i++)
    {
      vtkSMRepresentationProxy* repr =
        vtkSMRepresentationProxy::SafeDownCast(this->GetProducerProxy(i));
      if (repr)
      {
        repr->ViewUpdated(this);
      }
      else
      {
        // this->GetProducerProxy(i)->PostUpdateData();
      }
    }

    this->PostUpdateData();
  }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* proxy, int outputPort)
{
  assert("The session should be valid" && this->Session);

  vtkSMSourceProxy* producer = vtkSMSourceProxy::SafeDownCast(proxy);
  if ((producer == NULL) || (outputPort < 0) ||
    (static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort) ||
    (producer->GetSession() != this->GetSession()))
  {
    return NULL;
  }

  // Update with time from the view to ensure we have up-to-date data.
  double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
  producer->UpdatePipeline(view_time);

  const char* representationType = this->GetRepresentationType(producer, outputPort);
  if (!representationType)
  {
    return NULL;
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> p;
  p.TakeReference(pxm->NewProxy("representations", representationType));
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(p);
  if (repr)
  {
    repr->Register(this);
    return repr;
  }
  vtkWarningMacro(
    "Failed to create representation (representations," << representationType << ").");
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkSMViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer && static_cast<int>(producer->GetNumberOfOutputPorts()) > outputPort);

  // Process producer hints to see if indicates what type of representation
  // to create for this view.
  if (const char* reprName = vtkSMViewProxyNS::GetRepresentationNameFromHints(
        this->GetXMLName(), producer->GetHints(), outputPort))
  {
    return reprName;
  }

  // check if we have default representation name specified in XML.
  if (this->DefaultRepresentationName)
  {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    vtkSMProxy* prototype =
      pxm->GetPrototypeProxy("representations", this->DefaultRepresentationName);
    if (prototype)
    {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);

      if (acceptable)
      {
        return this->DefaultRepresentationName;
      }
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::CanDisplayData(vtkSMSourceProxy* producer, int outputPort)
{
  if (producer == NULL || outputPort < 0 ||
    static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort ||
    producer->GetSession() != this->GetSession())
  {
    return false;
  }

  const char* type = this->GetRepresentationType(producer, outputPort);
  if (type != NULL)
  {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    return (pxm->GetPrototypeProxy("representations", type) != NULL);
  }

  return false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::FindRepresentation(
  vtkSMSourceProxy* producer, int outputPort)
{
  vtkSMPropertyHelper helper(this, "Representations");
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));
    if (repr && repr->GetProperty("Input"))
    {
      vtkSMPropertyHelper helper2(repr, "Input");
      if (helper2.GetAsProxy() == producer &&
        static_cast<int>(helper2.GetOutputPort()) == outputPort)
      {
        return repr;
      }
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMViewProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
  {
    return 0;
  }

  const char* repr_name = element->GetAttribute("representation_name");
  if (repr_name)
  {
    this->SetDefaultRepresentationName(repr_name);
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindow(int magX, int magY)
{
  vtkSMViewProxyNS::CaptureHelper helper(this);
  if (auto img = helper.StereoCapture(magX, magY))
  {
    img->Register(this);
    return img.GetPointer();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindowSingle(int magX, int magY)
{
  if (this->ObjectsCreated)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "PrepareForScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }

  vtkImageData* capture = this->CaptureWindowInternal(magX, magY);

  if (this->ObjectsCreated)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "CleanupAfterScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }

  if (capture)
  {
    int position[2];
    vtkSMPropertyHelper(this, "ViewPosition").Get(position, 2);

    // Update image extents based on ViewPosition
    int extents[6];
    capture->GetExtent(extents);
    extents[0] += position[0] * magX;
    extents[1] += position[0] * magX;
    extents[2] += position[1] * magY;
    extents[3] += position[1] * magY;
    capture->SetExtent(extents);
  }
  return capture;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindowInternal(int magX, int magY)
{
  vtkRenderWindow* renWin = this->GetRenderWindow();
  if (!renWin)
  {
    return nullptr;
  }

  int swapBuffers = renWin->GetSwapBuffers();
  renWin->SwapBuffersOff();

  // this is needed to ensure that view gets setup correctly before go ahead to
  // capture the image.
  this->RenderForImageCapture();

  vtkNew<vtkSMViewProxyNS::WindowToImageFilter> w2i;
  w2i->SetInput(renWin);
  w2i->SetParent(this);
  w2i->SetScale(magX, magY);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff(); // WindowToImageFilter can re-render as needed too,
                            // we just don't require the first render.

  // Note how we simply called `Update` here. Since `WindowToImageFilter` calls
  // this->RenderForImageCapture() we don't have to worry too much even if it
  // gets called only on the client side (or root node in batch mode).
  w2i->Update();

  renWin->SetSwapBuffers(swapBuffers);

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMViewProxy::WriteImage(const char* filename, const char* writerName, int magnification)
{
  return this->WriteImage(filename, writerName, magnification, magnification);
}

//-----------------------------------------------------------------------------
int vtkSMViewProxy::WriteImage(const char* filename, const char* writerName, int magX, int magY)
{
  if (!filename || !writerName)
  {
    return vtkErrorCode::UnknownError;
  }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magX, magY));

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetSymmetricMPIMode())
  {
    return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
  }
  return vtkSMUtilities::SaveImage(shot, filename, writerName);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::SetTransparentBackground(bool val)
{
  vtkSMViewProxy::TransparentBackground = val;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::GetTransparentBackground()
{
  return vtkSMViewProxy::TransparentBackground;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::IsContextReadyForRendering()
{
  if (vtkRenderWindow* window = this->GetRenderWindow())
  {
    if (window->IsDrawable())
    {
      return true;
    }

    // If window is not drawable, we fire an event to notify the application
    // that we really need the OpenGL context. The application may use delays
    // etc to try to provide the context, if possible (see paraview/paraview#18945).
    this->InvokeEvent(vtkSMViewProxy::PrepareContextForRendering);
    return window->IsDrawable();
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::HideOtherRepresentationsIfNeeded(vtkSMProxy* repr)
{
  if (repr == NULL || this->GetHints() == NULL ||
    this->GetHints()->FindNestedElementByName("ShowOneRepresentationAtATime") == NULL)
  {
    return false;
  }

  vtkPVXMLElement* oneRepr =
    this->GetHints()->FindNestedElementByName("ShowOneRepresentationAtATime");
  const char* reprType = oneRepr->GetAttribute("type");

  if (reprType && strcmp(repr->GetXMLName(), reprType))
  {
    return false;
  }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  bool modified = false;
  vtkSMPropertyHelper helper(this, "Representations");
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMRepresentationProxy* arepr = vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));
    if (arepr && arepr != repr)
    {
      if (vtkSMPropertyHelper(arepr, "Visibility", /*quiet*/ true).GetAsInt() == 1 &&
        (!reprType || (reprType && !strcmp(arepr->GetXMLName(), reprType))))
      {
        controller->Hide(arepr, this);
        modified = true;
      }
    }
  }
  return modified;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RepresentationVisibilityChanged(vtkSMProxy*, bool)
{
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::GetLocalProcessSupportsInteraction()
{
  this->CreateVTKObjects();
  vtkPVView* pvview = vtkPVView::SafeDownCast(this->GetClientSideObject());
  return pvview ? pvview->GetLocalProcessSupportsInteraction() : false;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::MakeRenderWindowInteractor(bool quiet)
{
  if (this->GetInteractor() != NULL)
  {
    // all's setup already. nothing to do.
    return true;
  }
  if (!this->GetLocalProcessSupportsInteraction())
  {
    return false;
  }

  vtkRenderWindow* renWin = this->GetRenderWindow();
  if (!renWin)
  {
    if (!quiet)
    {
      vtkWarningMacro("Not a view that has a vtkRenderWindow. Cannot setup interactor.");
    }
    return false;
  }
  if (renWin->GetMapped())
  {
    if (!quiet)
    {
      vtkErrorMacro("Window is currently mapped. "
                    "Currently, interaction is only supported on unmapped windows.");
    }
    return false;
  }
  // in reality batch shouldn't have an interactor at all. However, to avoid the
  // mismatch in the vtkPVAxesWidget (orientation widget) when using pvpython or
  // pvbatch, we do create one. However, lets create a non-interactive
  // interactor in batch mode.
  vtkSmartPointer<vtkRenderWindowInteractor> iren;
  if (vtkProcessModule::GetProcessType() != vtkProcessModule::PROCESS_BATCH)
  {
    iren = renWin->MakeRenderWindowInteractor();
  }
  else
  {
    iren = vtkSmartPointer<vtkGenericRenderWindowInteractor>::New();
    // This initialize is essential. Otherwise vtkRenderWindow::Render causes
    // the interactor to initialize which in turn triggers a render!
    iren->Initialize();
  }
  this->SetupInteractor(iren);
  return this->GetInteractor() != NULL;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMViewProxy::FindView(vtkSMProxy* repr, const char* reggroup /*=views*/)
{
  if (!repr)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = repr->GetSessionProxyManager();
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin(reggroup); !iter->IsAtEnd(); iter->Next())
  {
    if (vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(iter->GetProxy()))
    {
      auto reprs = vtkSMProxyProperty::SafeDownCast(view->GetProperty("Representations"));
      auto hreprs = vtkSMProxyProperty::SafeDownCast(view->GetProperty("HiddenRepresentations"));
      if ((reprs != nullptr && reprs->IsProxyAdded(repr)) ||
        (hreprs != nullptr && hreprs->IsProxyAdded(repr)))
      {
        return view;
      }
    }
  }
  return nullptr;
}
