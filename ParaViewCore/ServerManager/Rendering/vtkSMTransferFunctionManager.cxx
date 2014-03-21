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
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"

#include <assert.h>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

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
  pxm->RegisterProxy("lookup_tables", proxyName.str().c_str(), proxy);
  proxy->FastDelete();
  return proxy;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionManager::GetOpacityTransferFunction(
  const char* arrayName, vtkSMSessionProxyManager* pxm)
{
  assert(arrayName != NULL && pxm != NULL);

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
  pxm->RegisterProxy("piecewise_functions", proxyName.str().c_str(), proxy);
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

  // this for some reason destroys the scalar bar properties all together.
  // What's going on ???
  //scalarBarProxy->ResetPropertiesToDefault();

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(scalarBarProxy);
  vtkSMPropertyHelper(scalarBarProxy, "LookupTable").Set(colorTransferFunction);
  // FIXME:
  vtkSMPropertyHelper(scalarBarProxy, "TitleFontSize").Set(6);
  vtkSMPropertyHelper(scalarBarProxy, "LabelFontSize").Set(6);

  controller->PostInitializeProxy(scalarBarProxy);

  pxm->RegisterProxy("scalar_bars", scalarBarProxy);
  scalarBarProxy->FastDelete();

  vtkSMPropertyHelper(view, "Representations").Add(scalarBarProxy);
  view->UpdateVTKObjects();

  // Place the scalar bar in the view.
  return scalarBarProxy;
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
