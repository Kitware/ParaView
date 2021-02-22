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
#include "vtkViewLayout.h"

#include "vtk_glew.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkPNGWriter.h"
#include "vtkPVLogger.h"
#include "vtkPVOptions.h"
#include "vtkPVProcessWindow.h"
#include "vtkPVView.h"
#include "vtkProcessModule.h"
#include "vtkProp.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTilesHelper.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

#include <vector>

class vtkViewLayoutProp : public vtkProp
{
public:
  static vtkViewLayoutProp* New();
  vtkTypeMacro(vtkViewLayoutProp, vtkProp);

  void SetLayout(vtkViewLayout* layout) { this->Layout = layout; }

  int RenderOverlay(vtkViewport* viewport) override
  {
    if (this->Layout)
    {
      this->Layout->Paint(viewport);
      return 1;
    }
    return 0;
  }

protected:
  vtkViewLayoutProp() = default;
  ~vtkViewLayoutProp() override = default;

  vtkWeakPointer<vtkViewLayout> Layout;

private:
  vtkViewLayoutProp(const vtkViewLayoutProp&) = delete;
  void operator=(const vtkViewLayoutProp&) = delete;
};
vtkStandardNewMacro(vtkViewLayoutProp);

class vtkViewLayout::vtkInternals
{
public:
  struct Item
  {
    vtkSmartPointer<vtkPVView> View;
    vtkVector4d Viewport;
    vtkVector2i TiledSize{ 0 };
    vtkVector2i TiledOrigin{ 0 };
    unsigned long RenderWindowStartEventObserverId = 0;
    unsigned long RenderWindowEndEventObserverId = 0;
  };

  std::vector<Item> Items;
  vtkTimeStamp UpdateLayoutTime;
  vtkWeakPointer<vtkRenderWindow> ActiveRenderWindow;

  static vtkViewLayout* LayoutToShowOnTileDisplay;
};

vtkViewLayout* vtkViewLayout::vtkInternals::LayoutToShowOnTileDisplay = nullptr;

vtkStandardNewMacro(vtkViewLayout);
//----------------------------------------------------------------------------
vtkViewLayout::vtkViewLayout()
  : Internals(new vtkViewLayout::vtkInternals())
  , InTileDisplay(false)
  , TileDimensions{ 0, 0 }
  , TileMullions{ 0, 0 }
  , InCave(false)
  , DisplayResults(false)
  , SeparatorWidth(0)
  , SeparatorColor{ 0, 0, 0 }
{
  if (auto processWindow = vtkPVProcessWindow::GetRenderWindow())
  {
    // if this process has window on which we may have to display the rendering
    // results for each view, then let's add an actor to it so we can do the
    // rendering as needed.
    this->Prop->SetLayout(this);

    auto ren = processWindow->GetRenderers()->GetFirstRenderer();
    ren->AddActor(this->Prop);

    auto options = vtkProcessModule::GetProcessModule()->GetOptions();
    if (options->GetIsInTileDisplay())
    {
      this->InTileDisplay = true;
      this->SetTileMullions(options->GetTileMullions());
      this->SetTileDimensions(options->GetTileDimensions());
      // TODO: skip paste-back on extra ranks if not debugging.
    }
    else if (options->GetIsInCave())
    {
      this->InCave = true;
    }

    this->DisplayResults =
      this->InCave || this->InTileDisplay || options->GetForceOnscreenRendering();
  }
}

//----------------------------------------------------------------------------
vtkViewLayout::~vtkViewLayout()
{
  if (auto processWindow = vtkPVProcessWindow::GetRenderWindow())
  {
    if (auto ren = processWindow->GetRenderers()->GetFirstRenderer())
    {
      ren->RemoveActor(this->Prop);
    }
  }

  if (vtkInternals::LayoutToShowOnTileDisplay == this)
  {
    vtkInternals::LayoutToShowOnTileDisplay = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkViewLayout::ShowOnTileDisplay()
{
  vtkInternals::LayoutToShowOnTileDisplay = this;
}

//----------------------------------------------------------------------------
void vtkViewLayout::RemoveAllViews()
{
  if (!this->DisplayResults)
  {
    return;
  }

  auto& internals = (*this->Internals);
  for (auto& item : internals.Items)
  {
    if (item.View->GetRenderWindow())
    {
      item.View->GetRenderWindow()->RemoveObserver(item.RenderWindowStartEventObserverId);
      item.View->GetRenderWindow()->RemoveObserver(item.RenderWindowEndEventObserverId);
    }
  }
  internals.Items.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkViewLayout::AddView(vtkPVView* view, const double* viewport)
{
  if (!this->DisplayResults)
  {
    return;
  }

  auto& internals = (*this->Internals);
  if (view)
  {
    vtkInternals::Item item;
    item.View = view;
    item.Viewport = vtkVector4d(viewport);
    // invert y
    item.Viewport[1] = 1.0 - viewport[3];
    item.Viewport[3] = 1.0 - viewport[1];
    if (auto renWin = view->GetRenderWindow())
    {
      item.RenderWindowStartEventObserverId = renWin->AddObserver(
        vtkViewLayout::RequestUpdateLayoutEvent, this, &vtkViewLayout::UpdateLayout);
      item.RenderWindowEndEventObserverId = renWin->AddObserver(
        vtkViewLayout::RequestUpdateDisplayEvent, this, &vtkViewLayout::UpdateDisplay);
      internals.Items.push_back(std::move(item));
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkViewLayout::AddView(vtkPVComparativeView*, const double*)
{
  // deferring for later.
}

//----------------------------------------------------------------------------
void vtkViewLayout::UpdateLayout(vtkObject* sender, unsigned long, void*)
{
  auto window = vtkRenderWindow::SafeDownCast(sender);
  assert(window);

  if (this->InTileDisplay)
  {
    this->UpdateLayoutForTileDisplay(window);
  }
  else if (this->InCave)
  {
    this->UpdateLayoutForCAVE(window);
  }
  else
  {
    // this would happen in remote rendering case.
    // nothing to do.
  }
}

//----------------------------------------------------------------------------
void vtkViewLayout::UpdateLayoutForCAVE(vtkRenderWindow*)
{
  auto& internals = (*this->Internals);
  assert(internals.Items.size() > 0);
  if (internals.UpdateLayoutTime > this->GetMTime())
  {
    return;
  }

  // get the window onto which we'll display the results on this process.
  // (cannot be null).
  auto processWindow = vtkPVProcessWindow::GetRenderWindow();
  assert(processWindow);

  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: UpdateLayoutForCAVE", vtkLogIdentifier(this));

  const auto size = processWindow->GetActualSize();

  // iterate over all views and setup window size and viewport for each view.
  for (auto& item : internals.Items)
  {
    // Here we're setting up the window size such that the entire view can
    // shown on  the current rank.
    if (auto renWin = item.View->GetRenderWindow())
    {
      renWin->SetSize(size);
    }
    // scale each renderer's viewport simply to the whole window since
    // in CAVE we only render one view at a time.
    double viewport[] = { 0, 0, 1, 1 };
    item.View->ScaleRendererViewports(viewport);
    item.TiledOrigin = vtkVector2i(0, 0);
    item.TiledSize = vtkVector2i(size);
  }

  internals.UpdateLayoutTime.Modified();
}

//----------------------------------------------------------------------------
void vtkViewLayout::UpdateLayoutForTileDisplay(vtkRenderWindow* vtkNotUsed(window))
{
  auto& internals = (*this->Internals);
  assert(internals.Items.size() > 0);
  if (internals.UpdateLayoutTime > this->GetMTime())
  {
    return;
  }

  // get the window onto which we'll display the results on this process.
  // (cannot be null).
  auto processWindow = vtkPVProcessWindow::GetRenderWindow();
  assert(processWindow);

  vtkVLogF(
    PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: UpdateLayoutForTileDisplay", vtkLogIdentifier(this));

  const int rank = vtkProcessModule::GetProcessModule()->GetPartitionId();

  // We are dealing with two types of tiling:
  // 1. The global tiling based on the -tdx,-tdy parameters passed to pvserver
  // 2. The view specific tiling. Since a view need not cover all of the global
  //    tiles (thanks to split layout), each view may be split among a
  //    potentially fewer set of tiles than the global tiles.
  vtkNew<vtkTilesHelper> helper;
  helper->SetTileDimensions(this->TileDimensions);
  helper->SetTileMullions(this->TileMullions);
  helper->SetTileWindowSize(processWindow->GetActualSize());

  vtkVector4d tile_viewport;
  if (!helper->GetTileViewport(rank, tile_viewport))
  {
    // rank is not rendering a tile, just set the viewport to full.
    tile_viewport[0] = tile_viewport[1] = 0.0;
    tile_viewport[2] = tile_viewport[3] = 1.0;
  }

  vtkVector2i tileSize, tileOrigin;
  helper->GetTiledSizeAndOrigin(rank, tileSize, tileOrigin);

  // iterate over all views and setup window size and viewport for each view.
  for (auto& item : internals.Items)
  {
    // Here we're setting up the window size such that the entire view can
    // shown on  the current rank.
    if (auto renWin = item.View->GetRenderWindow())
    {
      renWin->SetSize(helper->GetTileWindowSize());
      renWin->SetTileScale(this->TileDimensions);
      renWin->SetTileViewport(tile_viewport.GetData());
    }
    // scale each renderers viewport based on the global-viewport for this view.
    item.View->ScaleRendererViewports(item.Viewport.GetData());

    // compute tile size and origin for this view in pixels (makes pasting back easier).
    if (helper->GetTiledSizeAndOrigin(rank, item.TiledSize, item.TiledOrigin, item.Viewport))
    {
      item.TiledOrigin = item.TiledOrigin - tileOrigin;
    }
    else if (!helper->GetTileEnabled(rank))
    {
      // this is an extra rank, enable paste-back on it for debugging, if
      // needed.
      item.TiledOrigin = vtkVector2i(0, 0);
      item.TiledSize = vtkVector2i(helper->GetTileWindowSize());
    }
    else
    {
      item.TiledOrigin = vtkVector2i(0, 0);
      item.TiledSize = vtkVector2i(-1);
    }
  }

  internals.UpdateLayoutTime.Modified();
}

//----------------------------------------------------------------------------
void vtkViewLayout::UpdateDisplay(vtkObject* sender, unsigned long, void*)
{
  if (!this->DisplayResults)
  {
    return;
  }

  auto processWindow = vtkOpenGLRenderWindow::SafeDownCast(vtkPVProcessWindow::GetRenderWindow());
  assert(processWindow != nullptr);

  auto window = vtkRenderWindow::SafeDownCast(sender);
  assert(window);

  auto& internals = (*this->Internals);
  internals.ActiveRenderWindow = window;

  if (this->NeedsActiveStereo() && processWindow->GetStereoCapableWindow())
  {
    // the shared window only does stereo rendering for active stereo. For all
    // other modes, there's nothing specific to do since each view will compose
    // the stereo image separately.
    processWindow->SetStereoRender(true);
  }
  else if (processWindow->GetStereoRender())
  {
    processWindow->SetStereoRender(false);
  }

  if (!this->InTileDisplay && !this->InCave)
  {
    // this is batch mode or server mode when debugging rendering;
    // we are simply displaying the results from the last render;
    // instead of resizing the window on each render (which has issues) let's
    // resize to be at least as large as the window being rendered so
    // the results can be displayed.
    int new_size[2] = { std::max(processWindow->GetActualSize()[0], window->GetActualSize()[0]),
      std::max(processWindow->GetActualSize()[1], window->GetActualSize()[1]) };
    processWindow->SetSize(new_size);
  }

  processWindow->MakeCurrent();
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkViewLayout::UpdateDisplayForTileDisplay Start");
  processWindow->GetState()->Initialize(processWindow);
  processWindow->Render();
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkViewLayout::UpdateDisplayForTileDisplay End");

  // note, we don't restore active window to null here since we want it to be
  // preserved for `SaveAsPNG` if it gets called for testing purposes.
}

//----------------------------------------------------------------------------
void vtkViewLayout::Paint(vtkViewport* vp)
{
  if (this->InTileDisplay && this != vtkInternals::LayoutToShowOnTileDisplay)
  {
    return;
  }

  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: Paint", vtkLogIdentifier(this));

  auto& internals = (*this->Internals);
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkViewLayout::Paint Start");
  auto renderer = vtkOpenGLRenderer::SafeDownCast(vp);
  assert(renderer != nullptr);

  auto camera = renderer->GetActiveCamera();
  const bool leftEye = camera->GetLeftEye() != 0;

  auto window = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto ostate = window->GetState();

  vtkOpenGLState::ScopedglViewport scissorSaver(ostate);

  vtkNew<vtkTilesHelper> helper;
  helper->SetTileDimensions(this->TileDimensions);
  const int rank = vtkProcessModule::GetProcessModule()->GetPartitionId();

  for (auto& item : internals.Items)
  {
    if (!this->InTileDisplay || !helper->GetTileEnabled(rank))
    {
      // only paste-back active view if not in tile-display mode, or in
      // tile-display mode but on a rank not participating in the tile display
      // (in which case, it's being used for compositing alone)
      if (item.View->GetRenderWindow() != internals.ActiveRenderWindow)
      {
        continue;
      }
    }

    if (item.TiledSize[0] >= 0 && item.TiledSize[1] >= 0)
    {
      auto renWin = vtkOpenGLRenderWindow::SafeDownCast(item.View->GetRenderWindow());
      const int* size = renWin->GetActualSize();
      if (auto fbo = renWin ? renWin->GetRenderFramebuffer() : nullptr)
      {
        vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "paste back results from `%s`",
          item.View->GetLogName().c_str());
        // blit fbo to screen.
        if (this->InTileDisplay)
        {
          ostate->vtkglScissor(
            item.TiledOrigin[0], item.TiledOrigin[1], item.TiledSize[0], item.TiledSize[1]);
        }
        else
        {
          ostate->vtkglScissor(0, 0, size[0], size[1]);
        }
        if (fbo->GetFBOIndex())
        {
          vtkOpenGLCheckErrorMacro("Failed before paste-back");
          fbo->SaveCurrentBindingsAndBuffers(GL_READ_FRAMEBUFFER);
          fbo->Bind(GL_READ_FRAMEBUFFER);
          fbo->ActivateReadBuffer(leftEye ? 0 : 1);

          const int extents[] = { 0, size[0], 0, size[1] };
          fbo->Blit(extents, extents, GL_COLOR_BUFFER_BIT, GL_NEAREST);
          fbo->RestorePreviousBindingsAndBuffers(GL_READ_FRAMEBUFFER);
          vtkOpenGLCheckErrorMacro("Failed after paste-back");
        }
      }
    }
  }
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkViewLayout::Paint End");
}

//----------------------------------------------------------------------------
bool vtkViewLayout::NeedsActiveStereo() const
{
  const auto& internals = (*this->Internals);
  if (this->InTileDisplay)
  {
    // in tile display mode, we care about all views.
    for (auto& item : internals.Items)
    {
      auto window = item.View->GetRenderWindow();
      if (window && window->GetStereoCapableWindow() && window->GetStereoRender() &&
        window->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
      {
        return true;
      }
    }
  }
  else
  {
    // in non-tile display mode, we only care about the active view.
    auto window = internals.ActiveRenderWindow;
    return (window && window->GetStereoCapableWindow() && window->GetStereoRender() &&
      window->GetStereoType() == VTK_STEREO_CRYSTAL_EYES);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkViewLayout::SaveAsPNG(int rank, const char* filename)
{
  auto processWindow = vtkPVProcessWindow::GetRenderWindow();
  auto pm = vtkProcessModule::GetProcessModule();
  if (processWindow)
  {
    int result = 0;
    if (pm->GetPartitionId() == rank)
    {
      vtkLogF(INFO, "saving tile to %s", filename);
      vtkNew<vtkWindowToImageFilter> wif;
      wif->SetReadFrontBuffer(false);
      wif->SetShouldRerender(true);
      wif->SetInput(processWindow);
      processWindow->SetSwapBuffers(false);
      wif->Update();
      processWindow->SetSwapBuffers(true);

      vtkNew<vtkPNGWriter> writer;
      writer->SetFileName(filename);
      writer->SetInputDataObject(wif->GetOutput());
      writer->Write();
      result = 1;
    }

    int all_result = result;
    if (pm->GetNumberOfLocalPartitions() > 1)
    {
      pm->GetGlobalController()->Reduce(&result, &all_result, 1, 0, vtkCommunicator::MAX_OP);
    }
    return all_result > 0;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkViewLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SeparatorColor: " << this->SeparatorColor[0] << ", " << this->SeparatorColor[1]
     << ", " << this->SeparatorColor[2] << endl;
  os << indent << "SeparatorWidth: " << this->SeparatorWidth << endl;
}
