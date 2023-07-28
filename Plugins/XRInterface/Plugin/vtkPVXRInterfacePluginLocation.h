// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqXRInterfacePluginLocation
 * @brief   Stores ands returns runtime location of a plugin
 *
 */
#ifndef vtkPVXRInterfacePluginLocation_h
#define vtkPVXRInterfacePluginLocation_h

#include <string>

class vtkPVXRInterfacePluginLocation
{
public:
  // Callback when plugin is loaded.
  void StoreLocation(const char* location)
  {
    if (location)
    {
      this->PluginLocation = location;
    }
  }

  // global to get the location
  static std::string GetPluginLocation() { return vtkPVXRInterfacePluginLocation::PluginLocation; }

protected:
  static std::string PluginLocation;
};

#endif
