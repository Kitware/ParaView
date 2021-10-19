/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDynamicInitializerPluginInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDynamicInitializerPluginInterface
 *
 * vtkPVDynamicInitializerPluginInterface defines the interface needed to be
 * implemented by a plugin that needs to dynamically initialize its internal
 * state.
 *
 * It can be used to avoid depending on static initializers,
 * to initialize third-party libraries, etc.
 */

#ifndef vtkPVDynamicInitializerPluginInterface_h
#define vtkPVDynamicInitializerPluginInterface_h

#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVDynamicInitializerPluginInterface
{
public:
  virtual ~vtkPVDynamicInitializerPluginInterface();

  /**
   * The following method will be run as the plugin is loaded,
   * before server-manager or python plugin interfaces are run.
   */
  virtual void Initialize() = 0;
};
//@}

#endif

// VTK-HeaderTest-Exclude: vtkPVDynamicInitializerPluginInterface.h
