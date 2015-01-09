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
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTransferFunctionProxy.h"

#include <assert.h>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>
#include <set>

namespace
{
  vtkSMProxy* FindProxy(const char* groupName,
    const char* arrayName,
    vtkSMSessionProxyManager* pxm)
    {
    // Proxies are registered as  (arrayName).(ProxyXMLName).
    // In previous versions, they were registered as
    // (numComps).(arrayName).(ProxyXMLName). We dropped the numComps, but this
    // lookup should still match those old LUTs loaded from state.
    vtksys_ios::ostringstream expr;
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

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);

  // Look up array-specific transfer function
  vtksys_ios::ostringstream prefix;
  prefix << ".array_" << proxy->GetXMLGroup() << "." << arrayName;

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->GetProxySettings(prefix.str().c_str(), proxy);

  vtksys_ios::ostringstream proxyName;
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

  vtksys_ios::ostringstream proxyName;
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
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
      colorTransferFunction, view);
  if (scalarBarProxy)
    {
    return scalarBarProxy;
    }

  // Create a new scalar bar representation.
  vtkSMSessionProxyManager* pxm =
    colorTransferFunction->GetSessionProxyManager();
  scalarBarProxy = pxm->NewProxy("representations", "ScalarBarWidgetRepresentation");
  if (!scalarBarProxy)
    {
    vtkErrorMacro("Failed to create ScalarBarWidgetRepresentation proxy.");
    return NULL;
    }

  vtkNew<vtkSMParaViewPipelineController> controller;
  // we set these values before PreInitializeProxy() so that can be overridden
  // by user settings, if needed.
  //vtkSMPropertyHelper(scalarBarProxy, "TitleFontSize").Set(6);
  //vtkSMPropertyHelper(scalarBarProxy, "LabelFontSize").Set(6);

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
  vtkSMSessionProxyManager* pxm, bool extend)
{
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin("lookup_tables"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* lutProxy = iter->GetProxy();
    assert(lutProxy != NULL);
    if (vtkSMPropertyHelper(lutProxy, "LockScalarRange", true).GetAsInt() == 0)
      {
      double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
      if (vtkSMTransferFunctionProxy::ComputeDataRange(lutProxy, range))
        {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(
          lutProxy, range[0], range[1], extend);
        // BUG #0015076: Also reset the opacity function, if any.
        if (vtkSMProxy* sof = vtkSMPropertyHelper(
            lutProxy, "ScalarOpacityFunction", /*quiet*/true).GetAsProxy())
          {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(
            sof, range[0], range[1], extend);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionManager::UpdateScalarBars(
  vtkSMProxy* viewProxy, unsigned int mode)
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
  for (unsigned int cc=0, max=reprHelper.GetNumberOfElements(); cc < max; ++cc)
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
    for (proxysettype::const_iterator iter=currently_shown_scalar_bars.begin();
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
    for (proxysettype::const_iterator iter=colored_reprs.begin();
      iter != colored_reprs.end(); ++iter)
      {
      vtkSMPVRepresentationProxy::SetScalarBarVisibility(
        *iter, viewProxy, true);
      modified = true; // not really truthful here.
      }
    }
  return modified;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionManager::HideScalarBarIfNotNeeded(
  vtkSMProxy* lutProxy, vtkSMProxy* view)
{
  if (!lutProxy || !view)
    {
    return false;
    }

  vtkSMProxy* sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
    lutProxy, view);
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
  for (unsigned int cc=0, max=reprHelper.GetNumberOfElements(); cc < max; ++cc)
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
