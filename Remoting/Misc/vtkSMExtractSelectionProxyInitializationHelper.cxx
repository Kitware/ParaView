/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractSelectionProxyInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractSelectionProxyInitializationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>

vtkStandardNewMacro(vtkSMExtractSelectionProxyInitializationHelper);

//----------------------------------------------------------------------------
vtkSMExtractSelectionProxyInitializationHelper::vtkSMExtractSelectionProxyInitializationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMExtractSelectionProxyInitializationHelper::~vtkSMExtractSelectionProxyInitializationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMExtractSelectionProxyInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement* vtkNotUsed(element), vtkMTimeType vtkNotUsed(ts))
{
  assert(proxy != NULL);

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
