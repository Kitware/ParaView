/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   pqOpenVRPluginLocation
 * @brief   Stores ands returns runtime location of a plugin
 *
 */
#include <string>

class vtkPVOpenVRPluginLocation
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
  static std::string GetPluginLocation() { return vtkPVOpenVRPluginLocation::PluginLocation; }

protected:
  static std::string PluginLocation;
};
