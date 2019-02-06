/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunctionManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTransferFunctionManager.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionProxy.h"

#include <assert.h>
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
      assert(iter->GetProxy() != NULL);
      return iter->GetProxy();
    }
  }
  return NULL;
}
}

vtkObjectFactoryNewMacro(vtkSMTransferFunctionManager);
//----------------------------------------------------------------------------
vtkSMTransferFunctionManager::vtkSMTransferFunctionManager()
{
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionManager::~vtkSMTransferFunctionManager()
{
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetColorTransferFunction(
  const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  assert(arrayName != NULL && pxm != NULL);

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
    return NULL;
  }

  proxy->SetLogName((std::string("lut-for-") + std::string(arrayName)).c_str());

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);

  // Set the range reset mode based on the global setting
  vtkSMTransferFunctionProxy* tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  tfProxy->ResetRescaleModeToGlobalSetting();

  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // Load array-specific preset, if specified.
  std::string stdPresetsKey = ".standard_presets.";
  stdPresetsKey += arrayName;
  if (settings->HasSetting(stdPresetsKey.c_str()))
  {
    vtkSMTransferFunctionProxy::ApplyPreset(proxy,
      settings->GetSettingAsString(stdPresetsKey.c_str(), 0, "").c_str(),
      /*rescale=*/false);
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
  assert(arrayName != NULL && pxm != NULL);

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
    vtkErrorMacro("Failed to create PVLookupTable proxy.");
    return NULL;
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
vtkSMProxy* vtkSMTransferFunctionManager::GetScalarBarRepresentation(
  vtkSMProxy* colorTransferFunction, vtkSMProxy* view)
{
  if (colorTransferFunction == NULL || view == NULL ||
    !colorTransferFunction->IsA("vtkSMTransferFunctionProxy"))
  {
    return NULL;
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
    return NULL;
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
  for (iter->Begin("lookup_tables"); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxy* lutProxy = iter->GetProxy();
    assert(lutProxy != NULL);

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
    else if (rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY_AND_TIMESTEP && animating)
    {
      extend = true;
    }
    else if (rescaleMode == vtkSMTransferFunctionManager::RESET_ON_APPLY_AND_TIMESTEP && animating)
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
  typedef std::set<vtkSMProxy*> proxysettype;
  proxysettype currently_shown_scalar_bars;
  proxysettype luts;

  // colored_reprs are only those reprs that are uniquely colored with a LUT.
  // i.e. if two repr have the same lut, we only add the first one to this set.
  proxysettype colored_reprs;

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
      vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
      vtkSMProxy* lut = vtkSMPropertyHelper(proxy, "LookupTable", true).GetAsProxy();
      if (lut && luts.find(lut) == luts.end())
      {
        colored_reprs.insert(proxy);
        luts.insert(lut);
      }
    }
  }

  bool modified = false;

  if ((mode & HIDE_UNUSED_SCALAR_BARS) == HIDE_UNUSED_SCALAR_BARS)
  {
    // hide scalar-bars that point to lookup tables not in the luts set.
    for (proxysettype::const_iterator iter = currently_shown_scalar_bars.begin();
         iter != currently_shown_scalar_bars.end(); ++iter)
    {
      vtkSMProxy* sbProxy = (*iter);
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
    for (proxysettype::const_iterator iter = colored_reprs.begin(); iter != colored_reprs.end();
         ++iter)
    {
      vtkSMPVRepresentationProxy::SetScalarBarVisibility(*iter, viewProxy, true);
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

  bool ret = vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(
    lutProxy, vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(representation));
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

  vtkSMPropertyHelper visibilityHelper(sbProxy, "Visibility");
  if (visibilityHelper.GetAsInt() == 0)
  {
    // sb hidden.
    return false;
  }

  // build a list of all transfer functions used for scalar coloring in this
  // view and build a list of scalar bars currently shown in this view.
  vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = reprHelper.GetAsProxy(cc);
    if (vtkSMPropertyHelper(proxy, "Visibility", true).GetAsInt() == 1 &&
      vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
      vtkSMProxy* lut = vtkSMPropertyHelper(proxy, "LookupTable", true).GetAsProxy();
      if (lut == lutProxy)
      {
        // lut is being used. Don't hide the scalar bar.
        return false;
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
