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
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVView.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMUtilities.h"
#include "vtkSmartPointer.h"
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
  ~WindowToImageFilter() {}

  void Render() VTK_OVERRIDE
  {
    if (this->Parent)
    {
      this->Parent->RenderForImageCapture();
    }
  }

  vtkWeakPointer<vtkSMViewProxy> Parent;

private:
  WindowToImageFilter(const WindowToImageFilter&) VTK_DELETE_FUNCTION;
  void operator=(const WindowToImageFilter&) VTK_DELETE_FUNCTION;
};
vtkStandardNewMacro(WindowToImageFilter);
};

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

  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "Initialize"
         << static_cast<int>(this->GetGlobalID()) << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  vtkObject::SafeDownCast(this->GetClientSideObject())
    ->AddObserver(vtkPVView::ViewTimeChangedEvent, this, &vtkSMViewProxy::ViewTimeChanged);
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
    vtkClientServerStream stream;
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

class vtkSMViewProxy::vtkRendererSaveInfo
{
public:
  vtkRendererSaveInfo(vtkRenderer* renderer)
    : Gradient(renderer->GetGradientBackground())
    , Textured(renderer->GetTexturedBackground())
    , Red(renderer->GetBackground()[0])
    , Green(renderer->GetBackground()[1])
    , Blue(renderer->GetBackground()[2])
  {
  }

  const bool Gradient;
  const bool Textured;
  const double Red;
  const double Green;
  const double Blue;

private:
  vtkRendererSaveInfo(const vtkRendererSaveInfo&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRendererSaveInfo&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindow(int magnification)
{
  vtkRenderWindow* window = this->GetRenderWindow();

  if (window && this->TransparentBackground)
  {
    vtkRendererCollection* renderers = window->GetRenderers();
    vtkRenderer* renderer = renderers->GetFirstRenderer();
    while (renderer)
    {
      if (renderer->GetErase())
      {
        // Found a background-writing renderer.
        break;
      }

      renderer = renderers->GetNextItem();
    }

    if (!renderer)
    {
      // No renderer?
      return NULL;
    }

    vtkRendererSaveInfo* info = this->PrepareRendererBackground(renderer, 255, 255, 255, true);
    vtkImageData* captureWhite = this->CaptureWindowSingle(magnification);

    this->PrepareRendererBackground(renderer, 0, 0, 0, false);
    vtkImageData* captureBlack = this->CaptureWindowSingle(magnification);

    vtkImageData* capture = vtkImageData::New();
    capture->CopyStructure(captureWhite);
    capture->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

    const unsigned char* white;
    const unsigned char* black;
    unsigned char* out;
    vtkIdType whiteStep[3];
    vtkIdType blackStep[3];
    vtkIdType outStep[3];

    white = static_cast<const unsigned char*>(
      captureWhite->GetScalarPointerForExtent(captureWhite->GetExtent()));
    black = static_cast<const unsigned char*>(
      captureBlack->GetScalarPointerForExtent(captureBlack->GetExtent()));
    out = static_cast<unsigned char*>(capture->GetScalarPointerForExtent(capture->GetExtent()));

    captureWhite->GetIncrements(whiteStep[0], whiteStep[1], whiteStep[2]);
    captureBlack->GetIncrements(blackStep[0], blackStep[1], blackStep[2]);
    capture->GetIncrements(outStep[0], outStep[1], outStep[2]);

    int* extent = capture->GetExtent();

    for (int i = extent[4]; i <= extent[5]; ++i)
    {
      const unsigned char* whiteRow = white;
      const unsigned char* blackRow = black;
      unsigned char* outRow = out;

      for (int j = extent[2]; j <= extent[3]; ++j)
      {
        const unsigned char* whitePx = whiteRow;
        const unsigned char* blackPx = blackRow;
        unsigned char* outPx = outRow;

        for (int k = extent[0]; k <= extent[1]; ++k)
        {
          if (whitePx[0] == blackPx[0] && whitePx[1] == blackPx[1] && whitePx[2] == blackPx[2])
          {
            outPx[0] = whitePx[0];
            outPx[1] = whitePx[1];
            outPx[2] = whitePx[2];
            outPx[3] = 255;
          }
          else
          {
            // Some kind of translucency; use values from the black capture.
            // The opacity is the derived from the V difference of the HSV
            // colors.
            double whiteHSV[3];
            double blackHSV[3];

            vtkMath::RGBToHSV(whitePx[0] / 255., whitePx[1] / 255., whitePx[2] / 255., whiteHSV + 0,
              whiteHSV + 1, whiteHSV + 2);
            vtkMath::RGBToHSV(blackPx[0] / 255., blackPx[1] / 255., blackPx[2] / 255., blackHSV + 0,
              blackHSV + 1, blackHSV + 2);
            double alpha = 1. - (whiteHSV[2] - blackHSV[2]);

            outPx[0] = static_cast<unsigned char>(blackPx[0] / alpha);
            outPx[1] = static_cast<unsigned char>(blackPx[1] / alpha);
            outPx[2] = static_cast<unsigned char>(blackPx[2] / alpha);
            outPx[3] = static_cast<unsigned char>(255 * alpha);
          }

          whitePx += whiteStep[0];
          blackPx += blackStep[0];
          outPx += outStep[0];
        }

        whiteRow += whiteStep[1];
        blackRow += blackStep[1];
        outRow += outStep[1];
      }

      white += whiteStep[2];
      black += blackStep[2];
      out += outStep[2];
    }

    this->RestoreRendererBackground(renderer, info);

    captureWhite->Delete();
    captureBlack->Delete();
    return capture;
  }

  // Fall back to using no transparency.
  return this->CaptureWindowSingle(magnification);
}

//----------------------------------------------------------------------------
vtkSMViewProxy::vtkRendererSaveInfo* vtkSMViewProxy::PrepareRendererBackground(
  vtkRenderer* renderer, double r, double g, double b, bool save)
{
  vtkRendererSaveInfo* info = NULL;

  if (save)
  {
    info = new vtkRendererSaveInfo(renderer);
  }

  renderer->SetGradientBackground(false);
  renderer->SetTexturedBackground(false);
  renderer->SetBackground(r, g, b);

  return info;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RestoreRendererBackground(vtkRenderer* renderer, vtkRendererSaveInfo* info)
{
  renderer->SetGradientBackground(info->Gradient);
  renderer->SetTexturedBackground(info->Textured);
  renderer->SetBackground(info->Red, info->Green, info->Blue);

  delete info;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindowSingle(int magnification)
{
  if (this->ObjectsCreated)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "PrepareForScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
  }

  vtkImageData* capture = this->CaptureWindowInternal(magnification);

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
    for (int cc = 0; cc < 4; cc++)
    {
      extents[cc] += position[cc / 2] * magnification;
    }
    capture->SetExtent(extents);
  }
  return capture;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindowInternal(int magnification)
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
  w2i->SetMagnification(magnification);
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
  if (!filename || !writerName)
  {
    return vtkErrorCode::UnknownError;
  }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));

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

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  bool modified = false;
  vtkSMPropertyHelper helper(this, "Representations");
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMRepresentationProxy* arepr = vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));
    if (arepr && arepr != repr)
    {
      if (vtkSMPropertyHelper(arepr, "Visibility", /*quiet*/ true).GetAsInt() == 1)
      {
        controller->Hide(arepr, this);
        modified = true;
      }
    }
  }
  return modified;
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
