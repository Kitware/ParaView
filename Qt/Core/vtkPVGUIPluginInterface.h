/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGUIPluginInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPVGUIPluginInterface_h
#define vtkPVGUIPluginInterface_h

#include "pqCoreModule.h" // For export macro
#include <QObjectList>    // For the list of interfaces

/**
* vtkPVGUIPluginInterface defines the interface required by GUI plugins. This
* simply provides access to the GUI-component interfaces defined in this
* plugin.
*/
class PQCORE_EXPORT vtkPVGUIPluginInterface
{
public:
  virtual ~vtkPVGUIPluginInterface();
  virtual QObjectList interfaces() = 0;
};
Q_DECLARE_INTERFACE(vtkPVGUIPluginInterface, "com.kitware/paraview/guiplugin")
#endif
