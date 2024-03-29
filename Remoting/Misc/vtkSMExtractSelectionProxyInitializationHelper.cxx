// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMExtractSelectionProxyInitializationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>

vtkStandardNewMacro(vtkSMExtractSelectionProxyInitializationHelper);

//----------------------------------------------------------------------------
vtkSMExtractSelectionProxyInitializationHelper::vtkSMExtractSelectionProxyInitializationHelper() =
  default;

//----------------------------------------------------------------------------
vtkSMExtractSelectionProxyInitializationHelper::~vtkSMExtractSelectionProxyInitializationHelper() =
  default;

//----------------------------------------------------------------------------
void vtkSMExtractSelectionProxyInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement* vtkNotUsed(element), vtkMTimeType vtkNotUsed(ts))
{
  assert(proxy != nullptr);

  // See if the input proxy has a selection. If so, copy the selection.
  auto inputProperty = vtkSMInputProperty::SafeDownCast(proxy->GetProperty("Input"));
  if (inputProperty)
  {
    auto input = vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(inputProperty).GetAsProxy());
    unsigned int portNumber = inputProperty->GetOutputPortForConnection(0);

    auto selection = input->GetSelectionInput(portNumber);
    if (selection)
    {
      // Copy the selection to the extract selection proxy
      auto pxm = selection->GetSessionProxyManager();
      vtkSmartPointer<vtkSMSourceProxy> newSource;
      newSource.TakeReference(vtkSMSourceProxy::SafeDownCast(
        pxm->NewProxy(selection->GetXMLGroup(), selection->GetXMLName())));
      newSource->Copy(selection);
      newSource->UpdateVTKObjects();
      vtkSMPropertyHelper(proxy, "Selection").Set(newSource);
      proxy->UpdateVTKObjects();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMExtractSelectionProxyInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
