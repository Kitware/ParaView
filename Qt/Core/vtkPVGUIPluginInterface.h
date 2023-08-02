// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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

// VTK-HeaderTest-Exclude: vtkPVGUIPluginInterface.h
