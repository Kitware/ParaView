// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiComponentsDecorator.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"

#include <sstream>

//-----------------------------------------------------------------------------
pqMultiComponentsDecorator::pqMultiComponentsDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  std::stringstream ss(config->GetAttribute("components"));

  int n;
  while (ss >> n)
  {
    this->Components.push_back(n);
  }
}

//-----------------------------------------------------------------------------
bool pqMultiComponentsDecorator::canShowWidget(bool show_advanced) const
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  vtkPVArrayInformation* info = vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);

  if (info)
  {
    int nbComp = info->GetNumberOfComponents();

    if (std::find(this->Components.begin(), this->Components.end(), nbComp) ==
      this->Components.end())
    {
      return false;
    }
  }

  return this->Superclass::canShowWidget(show_advanced);
}
