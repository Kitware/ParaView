// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMTransferFunctionManager.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"

#include <cassert>
#include <set>
#include <sstream>
#include <vtksys/RegularExpression.hxx>

namespace
{
vtkSMProxy* FindProxy(const char* groupName, const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  // Proxies are registered as  (arrayName).(ProxyXMLName).
  // In previous versions, they were registered as
  // (numComps).(arrayName).(ProxyXMLName). We dropped the numComps, but this
  // lookup should still match those old LUTs loaded from state.
  std::ostringstream expr;
  expr << "^[0-9.]*" << arrayName << "\\.";
  vtksys::RegularExpression regExp(expr.str().c_str());
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin(groupName); !iter->IsAtEnd(); iter->Next())
  {
    const char* key = iter->GetKey();
    if (key && regExp.find(key))
    {
      assert(iter->GetProxy() != nullptr);
      return iter->GetProxy();
    }
  }
  return nullptr;
}
}

vtkObjectFactoryNewMacro(vtkSMTransferFunctionManager);
//----------------------------------------------------------------------------
vtkSMTransferFunctionManager::vtkSMTransferFunctionManager() = default;

//----------------------------------------------------------------------------
vtkSMTransferFunctionManager::~vtkSMTransferFunctionManager() = default;

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetColorTransferFunction(
  const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  assert(arrayName != nullptr && pxm != nullptr);

  // sanitize the arrayName. This is necessary since sometimes array names have
  // characters that can mess up regular expressions.
  std::string sanitizedArrayName = vtkSMCoreUtilities::SanitizeName(arrayName);
  arrayName = sanitizedArrayName.c_str();

  vtkSMProxy* proxy = FindProxy("lookup_tables", arrayName, pxm);
  if (proxy)
  {
    return proxy;
  }

  // Create a new one.
  proxy = pxm->NewProxy("lookup_tables", "PVLookupTable");
  if (!proxy)
  {
    vtkErrorMacro("Failed to create PVLookupTable proxy.");
    return nullptr;
  }

  proxy->SetLogName((std::string("lut-for-") + std::string(arrayName)).c_str());

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);

  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // Load array-specific preset, if specified.
  std::string stdPresetsKey = ".standard_presets.";
  stdPresetsKey += arrayName;
  if (settings->HasSetting(stdPresetsKey.c_str()))
  {
    const auto stdPreset = settings->GetSettingAsString(stdPresetsKey.c_str(), 0, "");
    vtkSMTransferFunctionProxy::ApplyPreset(proxy, stdPreset.c_str(), /*usePresetRange=*/false);
  }

  // Look up array-specific transfer function
  std::ostringstream prefix;
  prefix << ".array_" << proxy->GetXMLGroup() << "." << arrayName;

  settings->GetProxySettings(prefix.str().c_str(), proxy);

  std::ostringstream proxyName;
  proxyName << arrayName << ".PVLookupTable";
  if (proxy->GetProperty("ScalarOpacityFunction"))
  {
    vtkSMProxy* sof = this->GetOpacityTransferFunction(arrayName, pxm);
    if (sof)
    {
      sof->UpdateVTKObjects();
      vtkSMPropertyHelper(proxy, "ScalarOpacityFunction").Set(sof);

      // Since this function is initializing defaults for the lookup tables,
      // including defaults for the proxy property transfer functions, get the
      // proxy property to treat its current settings as the defaults.
      auto* prop = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("ScalarOpacityFunction"));
      prop->ResetDefaultsToCurrent();
    }
  }
  if (proxy->GetProperty("TransferFunction2D"))
  {
    vtkSMProxy* tf2d = this->GetTransferFunction2D(arrayName, pxm);
    if (tf2d)
    {
      tf2d->UpdateVTKObjects();
      vtkSMPropertyHelper(proxy, "TransferFunction2D").Set(tf2d);

      // Since this function is initializing defaults for the lookup tables,
      // including defaults for the proxy property transfer functions, get the
      // proxy property to treat its current settings as the defaults.
      auto* prop = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("TransferFunction2D"));
      prop->ResetDefaultsToCurrent();
    }
  }
  controller->PostInitializeProxy(proxy);
  controller->RegisterColorTransferFunctionProxy(proxy, proxyName.str().c_str());
  proxy->FastDelete();
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetOpacityTransferFunction(
  const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  assert(arrayName != nullptr && pxm != nullptr);

  // sanitize the arrayName. This is necessary since sometimes array names have
  // characters that can mess up regular expressions.
  std::string sanitizedArrayName = vtkSMCoreUtilities::SanitizeName(arrayName);
  arrayName = sanitizedArrayName.c_str();

  vtkSMProxy* proxy = FindProxy("piecewise_functions", arrayName, pxm);
  if (proxy)
  {
    return proxy;
  }

  // Create a new one.
  proxy = pxm->NewProxy("piecewise_functions", "PiecewiseFunction");
  if (!proxy)
  {
    vtkErrorMacro("Failed to create PiecewiseFunction proxy.");
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);
  controller->PostInitializeProxy(proxy);

  std::ostringstream proxyName;
  proxyName << arrayName << ".PiecewiseFunction";
  controller->RegisterOpacityTransferFunction(proxy, proxyName.str().c_str());
  proxy->FastDelete();
  proxy->UpdateVTKObjects();
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetTransferFunction2D(
  const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  return this->GetTransferFunction2D(arrayName, nullptr, pxm);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetTransferFunction2D(
  const char* arrayName, const char* array2Name, vtkSMSessionProxyManager* pxm)
{
  // ok to have the array2Name be empty (used when the gradient of the array is used as array2)
  assert(arrayName != nullptr && pxm != nullptr);

  // sanitize the arrayName. This is necessary since sometimes array names have
  // characters that can mess up regular expressions.
  std::string sanitizedArrayName = vtkSMCoreUtilities::SanitizeName(arrayName);
  arrayName = sanitizedArrayName.c_str();
  std::string sanitizedArray2Name = vtkSMCoreUtilities::SanitizeName(array2Name);
  array2Name = sanitizedArray2Name.c_str();
  std::ostringstream coupledArrayName;
  coupledArrayName << arrayName;
  if (array2Name && array2Name[0] != '\0')
  {
    coupledArrayName << "_" << array2Name;
  }
  vtkSMProxy* proxy = FindProxy("transfer_2d_functions", coupledArrayName.str().c_str(), pxm);
  if (proxy)
  {
    return proxy;
  }

  // Create a new one.
  proxy = pxm->NewProxy("transfer_2d_functions", "TransferFunction2D");
  if (!proxy)
  {
    vtkErrorMacro("Failed to create 2D TransferFunction proxy");
    return nullptr;
  }

  proxy->SetLogName((std::string("tf2d-for-") + coupledArrayName.str()).c_str());
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);
  controller->PostInitializeProxy(proxy);

  coupledArrayName << ".TransferFunction2D";
  controller->RegisterTransferFunction2D(proxy, coupledArrayName.str().c_str());
  proxy->FastDelete();
  proxy->UpdateVTKObjects();
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetScalarBarRepresentation(
  vtkSMProxy* colorTransferFunction, vtkSMProxy* view)
{
  if (colorTransferFunction == nullptr || view == nullptr ||
    !colorTransferFunction->IsA("vtkSMTransferFunctionProxy"))
  {
    return nullptr;
  }

  vtkSMProxy* scalarBarProxy =
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(colorTransferFunction, view);
  if (scalarBarProxy)
  {
    return scalarBarProxy;
  }

  // Create a new scalar bar representation.
  vtkSMSessionProxyManager* pxm = colorTransferFunction->GetSessionProxyManager();
  scalarBarProxy = pxm->NewProxy("representations", "ScalarBarWidgetRepresentation");
  if (!scalarBarProxy)
  {
    vtkErrorMacro("Failed to create ScalarBarWidgetRepresentation proxy.");
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  // we set these values before PreInitializeProxy() so that can be overridden
  // by user settings, if needed.
  // vtkSMPropertyHelper(scalarBarProxy, "TitleFontSize").Set(6);
  // vtkSMPropertyHelper(scalarBarProxy, "LabelFontSize").Set(6);

  controller->PreInitializeProxy(scalarBarProxy);
  vtkSMPropertyHelper(scalarBarProxy, "LookupTable").Set(colorTransferFunction);
  controller->PostInitializeProxy(scalarBarProxy);

  pxm->RegisterProxy("scalar_bars", scalarBarProxy);
  scalarBarProxy->FastDelete();

  vtkSMPropertyHelper(view, "Representations").Add(scalarBarProxy);
  view->UpdateVTKObjects();

  // Place the scalar bar in the view.
  return scalarBarProxy;
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionManager::ResetAllTransferFunctionRangesUsingCurrentData(
  vtkSMSessionProxyManager* pxm, bool animating)
{
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();

  vtkSMRepresentationProxy* representationProxy = nullptr;
  for (iter->Begin("representations"); !iter->IsAtEnd(); iter->Next())
  {
    representationProxy = vtkSMRepresentationProxy::SafeDownCast(iter->GetProxy());
  }

  for (iter->Begin("lookup_tables"); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxy* lutProxy = iter->GetProxy();

    assert(lutProxy != nullptr);

    int rescaleMode = vtkSMPropertyHelper(lutProxy, "AutomaticRescaleRangeMode", true).GetAsInt();

    bool extend = false;

    if (rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY && !animating)
    {
      extend = true;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::RESET_ON_APPLY && !animating)
    {
      extend = false;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::RESET_VISIBLE_ON_APPLY && !animating)
    {
      extend = false;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY_AND_TIMESTEP && animating)
    {
      extend = true;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::RESET_ON_APPLY_AND_TIMESTEP && animating)
    {
      extend = false;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::RESET_VISIBLE_ON_APPLY_AND_TIMESTEP &&
      animating)
    {
      extend = false;
    }
    else /*never*/
    {
      // Don't do any rescaling
      continue;
    }

    if (vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).GetAsInt() == 1)
    {
      vtkSMTransferFunctionProxy::ComputeAvailableAnnotations(lutProxy, extend);
    }
    else
    {
      if (rescaleMode == vtkSMTransferFunctionManager::RESET_VISIBLE_ON_APPLY_AND_TIMESTEP ||
        rescaleMode == vtkSMTransferFunctionManager::RESET_VISIBLE_ON_APPLY)
      {
        vtkSMViewProxy* activeView = nullptr;
        if (vtkSMProxySelectionModel* viewSM = pxm->GetSelectionModel("ActiveView"))
        {
          activeView = vtkSMViewProxy::SafeDownCast(viewSM->GetCurrentProxy());
        }

        vtkSMSourceProxy* activeSource = nullptr;
        if (vtkSMProxySelectionModel* sourceSM = pxm->GetSelectionModel("ActiveSources"))
        {
          activeSource = vtkSMSourceProxy::SafeDownCast(sourceSM->GetCurrentProxy());
        }

        // Deduce active representation from active source (if any) and active view
        vtkSMRepresentationProxy* activeRepr;
        if (!activeSource)
        {
          activeRepr = representationProxy;
        }
        else
        {
          vtkSMPropertyHelper inputHelper(activeSource, "Input", true);
          activeRepr = activeView->FindRepresentation(activeSource, inputHelper.GetOutputPort());
        }
        if (activeRepr && activeView)
        {
          vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(activeRepr, activeView);
        }
        return;
      }

      double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
      if (vtkSMTransferFunctionProxy::ComputeDataRange(lutProxy, range))
      {
        vtkSMCoreUtilities::AdjustRange(range);
        vtkSMTransferFunctionProxy::RescaleTransferFunction(lutProxy, range[0], range[1], extend);
        // BUG #0015076: Also reset the opacity function, if any.
        if (vtkSMProxy* sof =
              vtkSMPropertyHelper(lutProxy, "ScalarOpacityFunction", /*quiet*/ true).GetAsProxy())
        {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range[0], range[1], extend);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionManager::UpdateScalarBars(vtkSMProxy* viewProxy, unsigned int mode)
{
  std::set<vtkSMProxy*> currently_shown_scalar_bars;
  std::set<vtkSMProxy*> luts;

  // colored_reprs are only those reprs that are uniquely colored with a LUT.
  // i.e. if two repr have the same lut, we only add the first one to this set.
  std::set<vtkSMProxy*> colored_reprs;
  std::set<std::pair<vtkSMProxy*, std::string>> colored_reprs_blocks;

  // build a list of all transfer functions used for scalar coloring in this
  // view and build a list of scalar bars currently shown in this view.
  vtkSMPropertyHelper reprHelper(viewProxy, "Representations");
  for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = reprHelper.GetAsProxy(cc);
    if (vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(proxy) &&
      (vtkSMPropertyHelper(proxy, "Visibility").GetAsInt() == 1))
    {
      currently_shown_scalar_bars.insert(proxy);
    }
    else if (vtkSMPropertyHelper(proxy, "Visibility", true).GetAsInt() == 1 &&
      (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy) ||
        vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(proxy)))

    {
      vtkSMProxy* lut = vtkSMPropertyHelper(proxy, "LookupTable", true).GetAsProxy();
      if (lut && luts.find(lut) == luts.end())
      {
        colored_reprs.insert(proxy);
        luts.insert(lut);
      }
      const vtkSMPropertyHelper blockColorArrayNames(proxy, "BlockColorArrayNames", true);
      const vtkSMPropertyHelper blockLookUpTables(proxy, "BlockLookupTables", true);
      for (unsigned int i = 0; i < blockLookUpTables.GetNumberOfElements(); ++i)
      {
        vtkSMProxy* blockLut = blockLookUpTables.GetAsProxy(i);
        if (blockLut && luts.find(blockLut) == luts.end())
        {
          colored_reprs_blocks.insert(
            std::make_pair(proxy, blockColorArrayNames.GetAsString(3 * i)));
          luts.insert(blockLut);
        }
      }
    }
  }

  bool modified = false;

  if ((mode & HIDE_UNUSED_SCALAR_BARS) == HIDE_UNUSED_SCALAR_BARS)
  {
    // hide scalar-bars that point to lookup tables not in the luts set.
    for (auto& sbProxy : currently_shown_scalar_bars)
    {
      if (sbProxy &&
        (luts.find(vtkSMPropertyHelper(sbProxy, "LookupTable").GetAsProxy()) == luts.end()))
      {
        vtkSMPropertyHelper(sbProxy, "Visibility").Set(0);
        vtkSMPropertyHelper(sbProxy, "Enabled").Set(0);
        sbProxy->UpdateVTKObjects();
        modified = true;
      }
    }
  }

  // NOTE: currently_shown_scalar_bars at this point also include scalar bars
  // that we just hid.

  if ((mode & SHOW_USED_SCALAR_BARS) == SHOW_USED_SCALAR_BARS)
  {
    // create and show scalar-bar for LUTs if they don't already exist.
    // To do that, we iterate over reprs and turn on scalar bar. This is needed
    // to ensure that a new scalar bar is created, it gets the right title.
    for (auto& reprProxy : colored_reprs)
    {
      vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, viewProxy, true);
      modified = true; // not really truthful here.
    }
    for (auto& reprBlockProxy : colored_reprs_blocks)
    {
      vtkSMColorMapEditorHelper::SetBlockScalarBarVisibility(
        reprBlockProxy.first, viewProxy, reprBlockProxy.second, true);
      modified = true; // not really truthful here.
    }
  }
  return modified;
}
//----------------------------------------------------------------------------
bool vtkSMTransferFunctionManager::UpdateScalarBarsComponentTitle(
  vtkSMProxy* lutProxy, vtkSMProxy* representation)
{
  if (!lutProxy || !representation)
  {
    return false;
  }

  bool ret = false;
  if (auto result = vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(
        lutProxy, vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(representation)))
  {
    ret |= result;
  }
  if (representation->GetProperty("BlockColorArrayNames"))
  {
    vtkSMPropertyHelper blockColorArrayNames(representation, "BlockColorArrayNames");
    for (unsigned int i = 0; i < blockColorArrayNames.GetNumberOfElements(); i += 3)
    {
      if (auto result = vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(lutProxy,
            vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(
              representation, blockColorArrayNames.GetAsString(i))))
      {
        ret |= result;
      }
    }
  }
  SM_SCOPED_TRACE(CallFunction)
    .arg("UpdateScalarBarsComponentTitle")
    .arg(lutProxy)
    .arg(representation)
    .arg("comment", "Update a scalar bar component title.");
  return ret;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionManager::HideScalarBarIfNotNeeded(vtkSMProxy* lutProxy, vtkSMProxy* view)
{
  if (!lutProxy || !view)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("HideScalarBarIfNotNeeded")
    .arg(lutProxy)
    .arg(view)
    .arg("comment", "Hide the scalar bar for this color map if no visible data is colored by it.");

  vtkSMProxy* sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view);
  if (!sbProxy)
  {
    // sb hidden.
    return false;
  }

  const vtkSMPropertyHelper visibilityHelper(sbProxy, "Visibility");
  if (visibilityHelper.GetAsInt() == 0)
  {
    // sb hidden.
    return false;
  }

  // build a list of all transfer functions used for scalar coloring in this
  // view and build a list of scalar bars currently shown in this view.
  const vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = reprHelper.GetAsProxy(cc);
    if (vtkSMPropertyHelper(proxy, "Visibility", true).GetAsInt() == 1 &&
      (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy) ||
        vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(proxy)))
    {
      vtkSMProxy* lut = vtkSMPropertyHelper(proxy, "LookupTable", true).GetAsProxy();
      if (lut == lutProxy)
      {
        // lut is being used. Don't hide the scalar bar.
        return false;
      }
      // check for the block related lookup tables
      const vtkSMPropertyHelper blockLookUpTables(proxy, "BlockLookupTables", true);
      for (unsigned int i = 0; i < blockLookUpTables.GetNumberOfElements(); ++i)
      {
        if (blockLookUpTables.GetAsProxy(i) == lutProxy)
        {
          // lut is being used. Don't hide the scalar bar.
          return false;
        }
      }
    }
  }

  vtkSMPropertyHelper(sbProxy, "Visibility").Set(0);
  sbProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
