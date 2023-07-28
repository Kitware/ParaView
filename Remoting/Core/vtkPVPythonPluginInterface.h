// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVPythonPluginInterface
 *
 * vtkPVPythonPluginInterface defines the interface required by ParaView plugins
 * that add python modules to ParaView.
 */

#ifndef vtkPVPythonPluginInterface_h
#define vtkPVPythonPluginInterface_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include <string>                  // STL Header
#include <vector>                  // STL Header

class VTKREMOTINGCORE_EXPORT vtkPVPythonPluginInterface
{
public:
  virtual ~vtkPVPythonPluginInterface();

  virtual void GetPythonSourceList(std::vector<std::string>& modules,
    std::vector<std::string>& sources, std::vector<int>& package_flags) = 0;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVPythonPluginInterface.h
