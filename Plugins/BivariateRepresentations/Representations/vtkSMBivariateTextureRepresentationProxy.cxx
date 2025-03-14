// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMBivariateTextureRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMLink.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"

#include <array>
#include <string>

/**
 * NOTE FOR DEVELOPERS
 * Several aspects of the current scalar bar mechanism in ParaView could be reworked
 * in order to make it more flexible:
 * - Do not require having a LUT for displaying a scalar bar (hence having a LUT property
 * on the scalar bar actor)
 * - Do not require the scalar bar proxy name to be `ScalarBarRepresentation` in order to
 * retrieve it in the view; consider using maybe a dedicated group name instead
 *
 * Regarding this class specifically, consider adding a way to easily identify if it
 * corresponds to active internal representation. Having a specific event fired when the
 * internal representation changes in the composite representation could be very beneficial
 * as well. We have to deal with the CreateVTKObjects() which is also called when we update
 * properties.
 */

vtkStandardNewMacro(vtkSMBivariateTextureRepresentationProxy);

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::ViewUpdated(vtkSMProxy* view)
{
  this->CurrentView = view;
  this->Superclass::ViewUpdated(view);
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::CreateVTKObjects()
{
  if (!this->UpdatingPropertyInfo)
  {
    if (this->TexturedScalarBarProxy && this->CurrentView)
    {
      // Remove the textured scalar bar from the view. This will force
      // vtkSMTransferFunctionManager::GetScalarBarRepresentation to to create or retrieve a
      // standard scalar bar when switching to other (internal) representations.
      vtkSMPropertyHelper(this->CurrentView, "Representations")
        .Remove(this->TexturedScalarBarProxy);
      this->CurrentView->UpdateVTKObjects();
    }
  }

  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatingPropertyInfo = true;
  this->UpdatePropertyInformation();
  this->UpdatingPropertyInfo = false;

  this->CreateTexturedScalarBar();
  this->SetupPropertiesLinks();

  this->AddObserver(vtkCommand::UpdateDataEvent, this,
    &vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated);
}

//-----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::CreateTexturedScalarBar()
{
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* pxm = proxyManager->GetActiveSessionProxyManager();

  this->TexturedScalarBarProxy.TakeReference(
    pxm->NewProxy("CustomScalarBarWidgetRepresentation", "ScalarBarWidgetRepresentation"));
  if (!this->TexturedScalarBarProxy)
  {
    vtkDebugMacro(<< "Failed to create bar proxy.");
    return;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(this->TexturedScalarBarProxy);

  pxm->RegisterProxy("scalar_bars", this->TexturedScalarBarProxy);
}

//-----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::SetupPropertiesLinks()
{
  if (!this->TexturedScalarBarProxy)
  {
    vtkErrorMacro(<< "Unable to retrieve the textured scalar bar proxy.");
    return;
  }

  const std::array<std::array<std::string, 2>, 6> linkedProperties{
    { { "LookupTable", "LookupTable" }, { "BivariateTexture", "Texture" },
      { "FirstArrayRange", "FirstRange" }, { "SecondArrayRange", "SecondRange" },
      { "FirstArrayName", "FirstTitle" }, { "SecondArrayName", "SecondTitle" } }
  };

  // Link the texture property of the representation with the texture
  // property of the scalar bar
  this->Links.clear();
  this->Links.reserve(6);
  for (const auto& props : linkedProperties)
  {
    vtkNew<vtkSMPropertyLink> link;
    link->AddLinkedProperty(this, props[0].c_str(), vtkSMLink::INPUT);
    link->AddLinkedProperty(this->TexturedScalarBarProxy, props[1].c_str(), vtkSMLink::OUTPUT);
    this->Links.push_back(link);
  }
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::ShowTexturedScalarBar()
{
  if (!this->TexturedScalarBarProxy || !this->CurrentView)
  {
    return;
  }

  // Hide current scalar bar if needed
  vtkSMProxy* lutProxy = vtkSMPropertyHelper(this, "LookupTable").GetAsProxy();
  vtkSMScalarBarWidgetRepresentationProxy* currentSBProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));

  if (currentSBProxy)
  {
    vtkSMPropertyHelper(this->CurrentView, "Representations").Remove(currentSBProxy);
  }

  // Show the textured scalar bar
  vtkSMPropertyHelper(this->CurrentView, "Representations").Add(this->TexturedScalarBarProxy);
  this->CurrentView->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated(
  vtkObject*, unsigned long, void* vtkNotUsed(calldata))
{
  this->UpdatingPropertyInfo = true;
  this->UpdatePropertyInformation();
  this->UpdatingPropertyInfo = false;

  this->TexturedScalarBarProxy->UpdateVTKObjects();
  this->ShowTexturedScalarBar();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
