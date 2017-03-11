/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSaveScreenshotProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSaveScreenshotProxy.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

//============================================================================
/**
 * vtkSMSaveScreenshotProxy::vtkState helps save and then restore state for view
 * or layout. vtkState is the base class that defines the API that's implemented
 * by view and layout specific subclasses.
 *
 * The API comprises of bunch of `Set..` methods. The `Set` method typically
 * will save the current value, and restore it in the destructor.
 */
class vtkSMSaveScreenshotProxy::vtkState
{
  enum FontScalingModes
  {
    SCALE_FONTS_PROPORTIONALLY = 0,
    DISABLE_FONT_SCALING = 1,
  };

private:
  struct ViewStereoState
  {
    vtkSmartPointer<vtkSMViewProxy> View;
    int StereoRender;
    int StereoType;
  };

  struct ViewPPIState
  {
    vtkSmartPointer<vtkSMViewProxy> View;
    int ViewPPI;
  };

  vtkSMSessionProxyManager* ProxyManager;
  bool TransparentBackground;
  vtkSmartPointer<vtkSMProxy> OriginalColorPalette;
  std::vector<ViewStereoState> SavedStereoValues;
  std::vector<ViewPPIState> SavedPPIValues;

  vtkSMProxy* GetActivePalette() const
  {
    return this->ProxyManager->FindProxy("global_properties", "misc", "ColorPalette");
  }

  void SaveOriginalPalette()
  {
    if (this->OriginalColorPalette)
    {
      // already saved.
      return;
    }

    if (vtkSMProxy* colorPalette = this->GetActivePalette())
    {
      this->OriginalColorPalette.TakeReference(
        this->ProxyManager->NewProxy(colorPalette->GetXMLGroup(), colorPalette->GetXMLName()));
      this->OriginalColorPalette->Copy(colorPalette);
    }
  }

  void RestoreOriginalPalette()
  {
    if (this->OriginalColorPalette)
    {
      if (vtkSMProxy* colorPalette = this->GetActivePalette())
      {
        colorPalette->Copy(this->OriginalColorPalette);
      }
    }
  }

protected:
  int Magnification;
  vtkVector2i OriginalSize;

public:
  vtkState(vtkSMSessionProxyManager* pxm)
    : ProxyManager(pxm)
    , TransparentBackground(false)
    , Magnification(1)
    , OriginalSize(0, 0)
  {
    this->TransparentBackground = vtkSMViewProxy::GetTransparentBackground();
  }

  virtual ~vtkState()
  {
    vtkSMViewProxy::SetTransparentBackground(this->TransparentBackground);
    this->RestoreOriginalPalette();

    // Restore stereo modes, if changed.
    for (auto iter = this->SavedStereoValues.rbegin(); iter != this->SavedStereoValues.rend();
         ++iter)
    {
      vtkSMPropertyHelper(iter->View, "StereoRender", true).Set(iter->StereoRender);
      vtkSMPropertyHelper(iter->View, "StereoType", true).Set(iter->StereoType);
      iter->View->UpdateVTKObjects();
    }

    // Restore ppi, if changed.
    for (auto iter = this->SavedPPIValues.rbegin(); iter != this->SavedPPIValues.rend(); ++iter)
    {
      vtkSMPropertyHelper(iter->View, "PPI", true).Set(iter->ViewPPI);
      iter->View->UpdateVTKObjects();
    }
  }

  /**
   * Change the palette to \c palette. Saves the original palette to restore
   * later.
   */
  void SetColorPalette(const char* palette)
  {
    if (palette == NULL || palette[0] == '\0')
    {
      return;
    }

    vtkSmartPointer<vtkSMProxy> paletteProxy;
    paletteProxy.TakeReference(this->ProxyManager->NewProxy("palettes", palette));
    vtkSMProxy* activePalette = this->GetActivePalette();
    if (paletteProxy && activePalette)
    {
      this->SaveOriginalPalette();
      activePalette->Copy(paletteProxy);
    }
  }

  virtual vtkVector2i GetSize() const = 0;

  /**
   * Change the size. If the size is different from what this->GetSize()
   * returns, then this->Resize() is called and this->OriginalSize is updated.
   * Subclasses should restore to this->OriginalSize if it is valid in
   * destructor.
   */
  void SetSize(const vtkVector2i& size)
  {
    vtkVector2i cursize = this->GetSize();
    if (cursize != size)
    {
      this->OriginalSize = cursize;
      this->Magnification = vtkSMSaveScreenshotProxy::ComputeMagnification(size, cursize);
      this->Resize(cursize);
    }
  }

  void SetTransparentBackground(bool val) { vtkSMViewProxy::SetTransparentBackground(val); }
  virtual vtkSmartPointer<vtkImageData> CaptureImage() = 0;
  virtual void SetStereoMode(int mode) = 0;
  virtual void SetFontScaling(int mode) = 0;
  virtual void SetSeparatorWidth(int) {}
  virtual void SetSeparatorColor(double[3]) {}

protected:
  virtual void Resize(const vtkVector2i& size) = 0;

  void SetViewStereoMode(vtkSMViewProxy* view, int stereoMode)
  {
    assert(view);
    if (stereoMode <= -1)
    {
      return;
    }
    vtkSMPropertyHelper srender(view, "StereoRender", true);
    vtkSMPropertyHelper stype(view, "StereoType", true);
    if (stype.GetAsInt() != stereoMode)
    {
      // if mode changed, then we save it so we can restore at the end.
      ViewStereoState svalue;
      svalue.View = view;
      svalue.StereoType = stype.GetAsInt();
      svalue.StereoRender = srender.GetAsInt();
      this->SavedStereoValues.push_back(svalue);

      stype.Set(stereoMode);
      srender.Set(stereoMode == 0 ? 0 : 1);
      view->UpdateVTKObjects();
    }
  }

  void SetViewFontScaling(vtkSMViewProxy* view, int mode)
  {
    assert(view);
    if (mode == SCALE_FONTS_PROPORTIONALLY)
    {
      return; // nothing to do.
    }

    assert(mode == DISABLE_FONT_SCALING);

    vtkSMPropertyHelper vppi(view, "PPI");
    ViewPPIState dpis;
    dpis.View = view;
    dpis.ViewPPI = vppi.GetAsInt();
    this->SavedPPIValues.push_back(dpis);
    vppi.Set(vppi.GetAsInt() / this->Magnification);
    view->UpdateVTKObjects();
  }

private:
  vtkState(const vtkState&) VTK_DELETE_FUNCTION;
  void operator=(const vtkState&) VTK_DELETE_FUNCTION;
};

//============================================================================
class vtkSMSaveScreenshotProxy::vtkStateView : public vtkSMSaveScreenshotProxy::vtkState
{
  vtkWeakPointer<vtkSMViewProxy> View;
  typedef vtkSMSaveScreenshotProxy::vtkState Superclass;

public:
  vtkStateView(vtkSMViewProxy* view)
    : Superclass(view->GetSessionProxyManager())
    , View(view)
  {
  }

  ~vtkStateView()
  {
    if (this->OriginalSize[0] > 0 && this->OriginalSize[1] > 0)
    {
      this->Resize(this->OriginalSize);
    }
  }

  vtkVector2i GetSize() const VTK_OVERRIDE
  {
    vtkVector2i size;
    vtkSMPropertyHelper(this->View, "ViewSize").Get(size.GetData(), 2);
    return size;
  }

  vtkSmartPointer<vtkImageData> CaptureImage() VTK_OVERRIDE
  {
    vtkSmartPointer<vtkImageData> img;
    img.TakeReference(this->View->CaptureWindow(this->Magnification));
    return img;
  }

  void SetStereoMode(int mode) VTK_OVERRIDE { this->SetViewStereoMode(this->View, mode); }
  void SetFontScaling(int mode) VTK_OVERRIDE { this->SetViewFontScaling(this->View, mode); }

protected:
  void Resize(const vtkVector2i& size) VTK_OVERRIDE
  {
    vtkSMPropertyHelper(this->View, "ViewSize").Set(size.GetData(), 2);
    this->View->UpdateVTKObjects();
  }
};

//============================================================================
class vtkSMSaveScreenshotProxy::vtkStateLayout : public vtkSMSaveScreenshotProxy::vtkState
{
  int ShowWindowDecorations;
  vtkWeakPointer<vtkSMViewLayoutProxy> Layout;
  int OriginalSeparatorWidth;
  double OriginalSeparatorColor[3];
  typedef vtkSMSaveScreenshotProxy::vtkState Superclass;

public:
  vtkStateLayout(vtkSMViewLayoutProxy* layout)
    : Superclass(layout->GetSessionProxyManager())
    , Layout(layout)
  {
    // we're doing this here just for completeness, but in reality this should
    // happen on the UI side since the UI may need to refresh after decorations
    // are hidden.
    vtkSMPropertyHelper swdHelper(layout, "ShowWindowDecorations");
    this->ShowWindowDecorations = swdHelper.GetAsInt();
    swdHelper.Set(0);

    this->OriginalSeparatorWidth = layout->GetSeparatorWidth();
    layout->GetSeparatorColor(this->OriginalSeparatorColor);
  }

  ~vtkStateLayout()
  {
    vtkSMPropertyHelper(this->Layout, "ShowWindowDecorations").Set(this->ShowWindowDecorations);
    if (this->OriginalSize[0] > 0 && this->OriginalSize[1] > 0)
    {
      this->Resize(this->OriginalSize);
    }

    this->Layout->SetSeparatorWidth(this->OriginalSeparatorWidth);
    this->Layout->SetSeparatorColor(this->OriginalSeparatorColor);
    this->Layout->UpdateVTKObjects();
  }

  vtkVector2i GetSize() const VTK_OVERRIDE
  {
    vtkVector2i size;
    int ext[4];
    this->Layout->GetLayoutExtent(ext);
    size[0] = ext[1] - ext[0] + 1;
    size[1] = ext[3] - ext[2] + 1;
    return size;
  }

  vtkSmartPointer<vtkImageData> CaptureImage() VTK_OVERRIDE
  {
    vtkSmartPointer<vtkImageData> img;
    img.TakeReference(this->Layout->CaptureWindow(this->Magnification));
    return img;
  }

  void SetStereoMode(int mode) VTK_OVERRIDE
  {
    const std::vector<vtkSMViewProxy*> views = this->Layout->GetViews();
    for (auto iter = views.begin(); iter != views.end(); ++iter)
    {
      this->SetViewStereoMode(*iter, mode);
    }
  }

  void SetFontScaling(int mode) VTK_OVERRIDE
  {
    const std::vector<vtkSMViewProxy*> views = this->Layout->GetViews();
    for (auto iter = views.begin(); iter != views.end(); ++iter)
    {
      this->SetViewFontScaling(*iter, mode);
    }
  }

  void SetSeparatorWidth(int width) VTK_OVERRIDE { this->Layout->SetSeparatorWidth(width); }

  void SetSeparatorColor(double color[3]) VTK_OVERRIDE { this->Layout->SetSeparatorColor(color); }

protected:
  void Resize(const vtkVector2i& size) VTK_OVERRIDE { this->Layout->SetSize(size.GetData()); }
};

//============================================================================

vtkStandardNewMacro(vtkSMSaveScreenshotProxy);
//----------------------------------------------------------------------------
vtkSMSaveScreenshotProxy::vtkSMSaveScreenshotProxy()
  : State(NULL)
{
}

//----------------------------------------------------------------------------
vtkSMSaveScreenshotProxy::~vtkSMSaveScreenshotProxy()
{
  delete this->State;
  this->State = NULL;
}

//----------------------------------------------------------------------------
bool vtkSMSaveScreenshotProxy::WriteImage(const char* filename)
{
  vtkSMViewLayoutProxy* layout = this->GetLayout();
  vtkSMViewProxy* view = this->GetView();

  // view and layout are mutually exclusive.
  assert(layout == NULL || view == NULL);
  if (layout == NULL && view == NULL)
  {
    vtkErrorMacro("Cannot WriteImage without a view or layout.");
    return false;
  }

  SM_SCOPED_TRACE(SaveCameras)
    .arg("proxy", view != NULL ? static_cast<vtkSMProxy*>(view) : static_cast<vtkSMProxy*>(layout));

  SM_SCOPED_TRACE(SaveScreenshotOrAnimation)
    .arg("helper", this)
    .arg("filename", filename)
    .arg("view", view)
    .arg("layout", layout)
    .arg("mode_screenshot", 1);

  vtkSmartPointer<vtkImageData> img = this->CaptureImage();
  int quality = vtkSMPropertyHelper(this, "ImageQuality").GetAsInt();
  quality = std::max(0, quality);
  quality = std::min(100, quality);
  if (img && vtkProcessModule::GetProcessModule()->GetPartitionId() == 0)
  {
    return vtkSMUtilities::SaveImage(img.GetPointer(), filename, quality) == vtkErrorCode::NoError;
  }
  return false;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMSaveScreenshotProxy::CaptureImage()
{
  if (!this->Prepare())
  {
    vtkErrorMacro("Failed to prepare to capture image.");
    return NULL;
  }

  vtkSmartPointer<vtkImageData> img = this->CapturePreppedImage();

  this->Cleanup();
  return img;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMSaveScreenshotProxy::CapturePreppedImage()
{
  assert(this->State != NULL);
  return this->State->CaptureImage();
}

//----------------------------------------------------------------------------
int vtkSMSaveScreenshotProxy::ComputeMagnification(const vtkVector2i& targetSize, vtkVector2i& size)
{
  int magnification = 1;

  // If fullsize > viewsize, then magnification is involved.
  int temp = std::ceil(targetSize[0] / static_cast<double>(size[0]));
  magnification = std::max(temp, magnification);

  temp = std::ceil(targetSize[1] / static_cast<double>(size[1]));
  magnification = std::max(temp, magnification);
  size = targetSize / vtkVector2i(magnification);
  return magnification;
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy* vtkSMSaveScreenshotProxy::GetLayout()
{
  if (vtkSMPropertyHelper(this, "SaveAllViews").GetAsInt() != 0)
  {
    return vtkSMViewLayoutProxy::SafeDownCast(vtkSMPropertyHelper(this, "Layout").GetAsProxy());
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMSaveScreenshotProxy::GetView()
{
  if (vtkSMPropertyHelper(this, "SaveAllViews").GetAsInt() == 0)
  {
    return vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(this, "View").GetAsProxy());
  }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMSaveScreenshotProxy::Prepare()
{
  vtkSMViewLayoutProxy* layout = this->GetLayout();
  vtkSMViewProxy* view = this->GetView();

  // view and layout are mutually exclusive.
  assert(layout == NULL || view == NULL);
  if (layout == NULL && view == NULL)
  {
    vtkErrorMacro("Cannot `CaptureImage` without a view or layout.");
    return false;
  }

  assert(this->State == NULL);
  if (layout)
  {
    this->State = new vtkStateLayout(layout);
  }
  else if (view)
  {
    this->State = new vtkStateView(view);
  }

  // Update size.
  vtkVector2i targetSize;
  vtkSMPropertyHelper(this, "ImageResolution").Get(targetSize.GetData(), 2);
  this->State->SetSize(targetSize);
  this->State->SetTransparentBackground(
    vtkSMPropertyHelper(this, "TransparentBackground").GetAsInt() != 0);
  this->State->SetColorPalette(vtkSMPropertyHelper(this, "OverrideColorPalette").GetAsString());
  this->State->SetStereoMode(vtkSMPropertyHelper(this, "StereoMode").GetAsInt());
  this->State->SetFontScaling(vtkSMPropertyHelper(this, "FontScaling").GetAsInt());
  this->State->SetSeparatorWidth(vtkSMPropertyHelper(this, "SeparatorWidth").GetAsInt());

  double scolor[3];
  vtkSMPropertyHelper(this, "SeparatorColor").Get(scolor, 3);
  this->State->SetSeparatorColor(scolor);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSaveScreenshotProxy::Cleanup()
{
  assert(this->State != NULL);
  delete this->State;
  this->State = NULL;
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSaveScreenshotProxy::UpdateSaveAllViewsPanelVisibility()
{
  vtkSMViewLayoutProxy* layout =
    vtkSMViewLayoutProxy::SafeDownCast(vtkSMPropertyHelper(this, "Layout").GetAsProxy());
  if (layout == NULL || (layout->GetViews().size() <= 1))
  {
    this->GetProperty("SaveAllViews")->SetPanelVisibility("never");
    this->GetProperty("SeparatorWidth")->SetPanelVisibility("never");
    this->GetProperty("SeparatorColor")->SetPanelVisibility("never");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMSaveScreenshotProxy::CaptureImage(
  vtkSMViewProxy* view, const vtkVector2i& size)
{
  if (!view || size[0] <= 0 || size[1] <= 0)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "SaveScreenshot"));
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(proxy);
  if (!shProxy)
  {
    vtkGenericWarningMacro("Failed to create 'SaveScreenshot' proxy.");
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(shProxy);
  vtkSMPropertyHelper(shProxy, "View").Set(view);
  vtkSMPropertyHelper(shProxy, "SaveAllViews").Set(0);
  vtkSMPropertyHelper(shProxy, "ImageResolution").Set(size.GetData(), 2);
  controller->PostInitializeProxy(shProxy);
  return shProxy->CaptureImage();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMSaveScreenshotProxy::CaptureImage(
  vtkSMViewLayoutProxy* layout, const vtkVector2i& size)
{
  if (!layout || size[0] <= 0 || size[1] <= 0)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = layout->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "SaveScreenshot"));
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(proxy);
  if (!shProxy)
  {
    vtkGenericWarningMacro("Failed to create 'SaveScreenshot' proxy.");
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(shProxy);
  vtkSMPropertyHelper(shProxy, "Layout").Set(layout);
  vtkSMPropertyHelper(shProxy, "SaveAllViews").Set(1);
  vtkSMPropertyHelper(shProxy, "ImageResolution").Set(size.GetData(), 2);
  controller->PostInitializeProxy(shProxy);
  return shProxy->CaptureImage();
}

//----------------------------------------------------------------------------
void vtkSMSaveScreenshotProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
