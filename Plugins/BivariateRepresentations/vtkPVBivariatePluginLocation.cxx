// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVBivariatePluginLocation.h"

std::string vtkPVBivariatePluginLocation::PluginLocation{};

//-----------------------------------------------------------------------------
void vtkPVBivariatePluginLocation::StoreLocation(const char* location)
{
  if (location)
  {
    this->PluginLocation = location;
  }
}
