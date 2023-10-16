// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVBivariatePluginLocation
 * @brief   Stores ands returns runtime location of
 * the vtkPVBivariatePluginLocation plugin
 */
#ifndef vtkPVBivariatePluginLocation_h
#define vtkPVBivariatePluginLocation_h

#include <string>

class vtkPVBivariatePluginLocation
{
public:
  // Callback when plugin is loaded.
  void StoreLocation(const char* location);

  // Return the plugin location
  static std::string GetPluginLocation() { return vtkPVBivariatePluginLocation::PluginLocation; }

protected:
  static std::string PluginLocation;
};

#endif
