// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiComponentsDecorator.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"

#if VTK_MODULE_ENABLE_ParaView_RemotingViews
#include "vtkSMColorMapEditorHelper.h"
#endif

#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiComponentsDecorator);

//-----------------------------------------------------------------------------
vtkMultiComponentsDecorator::vtkMultiComponentsDecorator() = default;

//-----------------------------------------------------------------------------
vtkMultiComponentsDecorator::~vtkMultiComponentsDecorator() = default;

//-----------------------------------------------------------------------------
void vtkMultiComponentsDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << " Components : ";
  for (int n : this->Components)
  {
    os << n << " ";
  }
  os << "\n";
}

//-----------------------------------------------------------------------------
void vtkMultiComponentsDecorator::Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(config, proxy);
  std::stringstream ss(config->GetAttribute("components"));

  int n;
  while (ss >> n)
  {
    this->Components.push_back(n);
  }
}

//-----------------------------------------------------------------------------
bool vtkMultiComponentsDecorator::CanShow(bool show_advanced) const
{
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  vtkPVArrayInformation* info =
    vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(this->Proxy());

  if (info)
  {
    int nbComp = info->GetNumberOfComponents();

    if (std::find(this->Components.begin(), this->Components.end(), nbComp) ==
      this->Components.end())
    {
      return false;
    }
  }
#endif

  return this->Superclass::CanShow(show_advanced);
}
