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
#include "vtkImageWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <sstream>
#include <vtksys/SystemTools.hxx>

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
  vtkVector2i Magnification;
  vtkVector2i OriginalSize;

public:
  vtkState(vtkSMSessionProxyManager* pxm)
    : ProxyManager(pxm)
    , TransparentBackground(false)
    , Magnification(1, 1)
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
      bool approx = false;
      this->Magnification =
        vtkSMSaveScreenshotProxy::GetScaleFactorsAndSize(size, cursize, &approx);
      if (approx)
      {
        const vtkVector2i approxSize = cursize * this->Magnification;
        vtkGenericWarningMacro(<< "Cannot render at '(" << size[0] << ", " << size[1]
                               << ")'. Using "
                               << "'(" << approxSize[0] << ", " << approxSize[1] << ")'");
      }
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
    // not entirely sure how to best scale ppi.
    vppi.Set(vppi.GetAsInt() / std::max(this->Magnification[0], this->Magnification[1]));
    view->UpdateVTKObjects();
  }

private:
  vtkState(const vtkState&) = delete;
  void operator=(const vtkState&) = delete;
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

  ~vtkStateView() override
  {
    if (this->OriginalSize[0] > 0 && this->OriginalSize[1] > 0)
    {
      this->Resize(this->OriginalSize);
    }
  }

  vtkVector2i GetSize() const override
  {
    vtkVector2i size;
    vtkSMPropertyHelper(this->View, "ViewSize").Get(size.GetData(), 2);
    return size;
  }

  vtkSmartPointer<vtkImageData> CaptureImage() override
  {
    vtkSmartPointer<vtkImageData> img;
    img.TakeReference(this->View->CaptureWindow(this->Magnification[0], this->Magnification[1]));
    return img;
  }

  void SetStereoMode(int mode) override { this->SetViewStereoMode(this->View, mode); }
  void SetFontScaling(int mode) override { this->SetViewFontScaling(this->View, mode); }

protected:
  void Resize(const vtkVector2i& size) override
  {
    vtkSMPropertyHelper(this->View, "ViewSize").Set(size.GetData(), 2);
    this->View->UpdateVTKObjects();
  }
};

//============================================================================
class vtkSMSaveScreenshotProxy::vtkStateLayout : public vtkSMSaveScreenshotProxy::vtkState
{
  int PreviewMode[2];
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
    vtkSMPropertyHelper swdHelper(layout, "PreviewMode");
    swdHelper.Get(this->PreviewMode, 2);

    this->OriginalSeparatorWidth = vtkSMPropertyHelper(layout, "SeparatorWidth").GetAsInt();
    vtkSMPropertyHelper(layout, "SeparatorColor").Get(this->OriginalSeparatorColor, 3);
  }

  ~vtkStateLayout() override
  {
    vtkSMPropertyHelper(this->Layout, "PreviewMode").Set(this->PreviewMode, 2);
    vtkSMPropertyHelper(this->Layout, "SeparatorWidth").Set(this->OriginalSeparatorWidth);
    vtkSMPropertyHelper(this->Layout, "SeparatorColor").Set(this->OriginalSeparatorColor, 3);
    this->Layout->UpdateVTKObjects();

    if (this->OriginalSize[0] > 0 && this->OriginalSize[1] > 0)
    {
      this->Resize(this->OriginalSize);
    }
  }

  vtkVector2i GetSize() const override { return this->Layout->GetSize(); }

  vtkSmartPointer<vtkImageData> CaptureImage() override
  {
    vtkSmartPointer<vtkImageData> img;
    img.TakeReference(this->Layout->CaptureWindow(this->Magnification[0], this->Magnification[1]));
    return img;
  }

  void SetStereoMode(int mode) override
  {
    const std::vector<vtkSMViewProxy*> views = this->Layout->GetViews();
    for (auto iter = views.begin(); iter != views.end(); ++iter)
    {
      this->SetViewStereoMode(*iter, mode);
    }
  }

  void SetFontScaling(int mode) override
  {
    const std::vector<vtkSMViewProxy*> views = this->Layout->GetViews();
    for (auto iter = views.begin(); iter != views.end(); ++iter)
    {
      this->SetViewFontScaling(*iter, mode);
    }
  }

  void SetSeparatorWidth(int width) override
  {
    vtkSMPropertyHelper(this->Layout, "SeparatorWidth").Set(width);
    this->Layout->UpdateVTKObjects();
  }

  void SetSeparatorColor(double color[3]) override
  {
    vtkSMPropertyHelper(this->Layout, "SeparatorColor").Set(color, 3);
    this->Layout->UpdateVTKObjects();
  }

protected:
  void Resize(const vtkVector2i& size) override { this->Layout->SetSize(size.GetData()); }
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

  auto format = this->GetFormatProxy(filename);
  if (!format)
  {
    vtkErrorMacro("Failed to determine format for '" << filename << "'");
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
  if (img && vtkProcessModule::GetProcessModule()->GetPartitionId() == 0)
  {
    auto writer = vtkImageWriter::SafeDownCast(format->GetClientSideObject());
    if (writer)
    {
      vtkTimerLog::MarkStartEvent("Write image to disk");
      writer->SetFileName(filename);
      writer->SetInputData(img);
      writer->Write();
      writer->SetInputData(nullptr);
      vtkTimerLog::MarkEndEvent("Write image to disk");
      return writer->GetErrorCode() == vtkErrorCode::NoError;
    }
    else
    {
      vtkErrorMacro("Format writer not a 'vtkImageWriter'. "
                    " Failed to write '"
        << filename << "'.");
    }
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
namespace
{
int computeGCD(int a, int b)
{
  return b == 0 ? a : computeGCD(b, a % b);
}

std::set<int> computeFactors(int num)
{
  std::set<int> result;
  const int sroot = std::sqrt(num);
  for (int cc = 1; cc <= sroot; ++cc)
  {
    if (num % cc == 0)
    {
      result.insert(cc);
      if (cc * cc != num)
      {
        result.insert(num / cc);
      }
    }
  }
  return result;
}
}

//----------------------------------------------------------------------------
vtkVector2i vtkSMSaveScreenshotProxy::GetScaleFactorsAndSize(
  const vtkVector2i& targetSize, vtkVector2i& size, bool* approximate)
{
  if (approximate)
  {
    *approximate = false;
  }

  if (targetSize[0] <= size[0] && targetSize[1] <= size[1])
  {
    // easy! It just fits.
    size = targetSize;
    return vtkVector2i(1, 1);
  }

  // First we need to see if we can find a magnification factor that preserves
  // aspect ratio. This is the best magnification factor. Do that, we get the
  // GCD for the target width and height and then see if factors of the GCD are
  // a good match.
  const int gcd = computeGCD(targetSize[0], targetSize[1]);
  if (gcd > 1)
  {
    const auto factors = computeFactors(gcd);
    for (auto fiter = factors.begin(); fiter != factors.end(); ++fiter)
    {
      const int magnification = *fiter;
      vtkVector2i potentialSize = targetSize / vtkVector2i(magnification, magnification);
      if (potentialSize[0] > 1 && potentialSize[1] > 1 && potentialSize[0] <= size[0] &&
        potentialSize[1] <= size[1])
      {
        // found a good fit that's non-trivial.
        size = potentialSize;
        return vtkVector2i(magnification, magnification);
      }
    }
  }

  // Next, try to find scale factors at the cost of preserving aspect ratios
  // since that's not possible. For this, we don't worry about GCD. Instead deal
  // with each dimension separately, finding factors for target size and seeing
  // if we can find a good scale factor.
  vtkVector2i magnification;
  for (int cc = 0; cc < 2; ++cc)
  {
    if (targetSize[cc] > size[cc])
    {
      // first, do a quick guess.
      magnification[cc] = std::ceil(targetSize[cc] / static_cast<double>(size[cc]));

      // now for a more accurate magnification; it may not be possible to find one,
      // and hence we first do an approximate calculation.
      const auto factors = computeFactors(targetSize[cc]);
      // Do not resize the image to less than half of the original size
      int minSize = std::max(1, size[cc] / 2);
      for (auto fiter = factors.begin(); fiter != factors.end(); ++fiter)
      {
        const int potentialSize = targetSize[cc] / *fiter;
        if (potentialSize > minSize && potentialSize <= size[cc])
        {
          // kaching!
          magnification[cc] = *fiter;
          break;
        }
      }
      size[cc] = targetSize[cc] / magnification[cc];
    }
    else
    {
      size[cc] = targetSize[cc];
      magnification[cc] = 1;
    }
  }

  if (approximate != nullptr)
  {
    *approximate = (size * magnification != targetSize);
  }

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
  this->State->SetTransparentBackground(
    vtkSMPropertyHelper(this, "TransparentBackground").GetAsInt() != 0);
  this->State->SetColorPalette(vtkSMPropertyHelper(this, "OverrideColorPalette").GetAsString());
  this->State->SetStereoMode(vtkSMPropertyHelper(this, "StereoMode").GetAsInt());
  this->State->SetFontScaling(vtkSMPropertyHelper(this, "FontScaling").GetAsInt());
  this->State->SetSeparatorWidth(vtkSMPropertyHelper(this, "SeparatorWidth").GetAsInt());
  double scolor[3];
  vtkSMPropertyHelper(this, "SeparatorColor").Get(scolor, 3);
  this->State->SetSeparatorColor(scolor);

  this->State->SetSize(targetSize);
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
void vtkSMSaveScreenshotProxy::UpdateDefaultsAndVisibilities(const char* filename)
{
  if (filename)
  {
    // pick correct "format" as the default.
    if (auto proxy = this->GetFormatProxy(filename))
    {
      vtkSMPropertyHelper(this, "Format").Set(proxy);
    }
  }

  vtkSMViewLayoutProxy* layout =
    vtkSMViewLayoutProxy::SafeDownCast(vtkSMPropertyHelper(this, "Layout").GetAsProxy());
  if (layout == NULL || (layout->GetViews().size() <= 1))
  {
    this->GetProperty("SaveAllViews")->SetPanelVisibility("never");
    this->GetProperty("SeparatorWidth")->SetPanelVisibility("never");
    this->GetProperty("SeparatorColor")->SetPanelVisibility("never");
  }
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

namespace detail
{
std::pair<std::string, std::vector<std::string> > GetFormatOptions(vtkSMProxy* proxy)
{
  using pair_type = std::pair<std::string, std::vector<std::string> >;
  vtkPVXMLElement* hints =
    proxy->GetHints() ? proxy->GetHints()->FindNestedElementByName("FormatOptions") : nullptr;
  if (hints && hints->GetAttribute("extensions") && hints->GetAttribute("file_description"))
  {
    const std::string desc(hints->GetAttribute("file_description"));
    const auto exts = vtksys::SystemTools::SplitString(hints->GetAttribute("extensions"), ' ');
    return pair_type(desc, exts);
  }
  return pair_type();
}
}

//----------------------------------------------------------------------------
std::string vtkSMSaveScreenshotProxy::GetFileFormatFilters()
{
  std::ostringstream str;
  auto pxm = this->GetSessionProxyManager();
  const auto pld = this->GetProperty("Format")->FindDomain<vtkSMProxyListDomain>();
  for (const auto& proxyType : pld->GetProxyTypes())
  {
    if (auto formatProxy =
          pxm->GetPrototypeProxy(proxyType.GroupName.c_str(), proxyType.ProxyName.c_str()))
    {
      const auto options = detail::GetFormatOptions(formatProxy);
      if (options.second.size() == 0)
      {
        continue;
      }
      if (str.tellp() != std::ostringstream::pos_type(0))
      {
        str << ";;";
      }
      str << options.first << " (";
      bool add_space = false;
      for (const auto& anext : options.second)
      {
        if (add_space)
        {
          str << " ";
        }
        str << "*." << anext;
        add_space = true;
      }
      str << ")";
    }
  }
  return str.str();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSaveScreenshotProxy::GetFormatProxy(const std::string& filename)
{
  auto extension = vtksys::SystemTools::GetFilenameLastExtension(filename);
  extension.erase(0, 1);
  if (extension.size() == 0)
  {
    vtkErrorMacro("Unknown file format for '" << filename << "'.");
  }
  auto pxm = this->GetSessionProxyManager();
  const auto pld = this->GetProperty("Format")->FindDomain<vtkSMProxyListDomain>();
  for (const auto& proxyType : pld->GetProxyTypes())
  {
    if (auto formatProxy =
          pxm->GetPrototypeProxy(proxyType.GroupName.c_str(), proxyType.ProxyName.c_str()))
    {
      const auto options = detail::GetFormatOptions(formatProxy);
      if (options.second.size() > 0)
      {
        const auto& exts = options.second;
        auto iter = std::find(exts.begin(), exts.end(), extension);
        if (iter != exts.end())
        {
          auto proxy = pld->FindProxy(proxyType.GroupName.c_str(), proxyType.ProxyName.c_str());
          assert(proxy != nullptr);
          return proxy;
        }
      }
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMSaveScreenshotProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
