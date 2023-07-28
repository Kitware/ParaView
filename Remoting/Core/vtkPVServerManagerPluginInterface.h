// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVServerManagerPluginInterface
 *
 * vtkPVServerManagerPluginInterface defines the interface needed to be
 * implemented by a server-manager plugin i.e. a plugin that adds new
 * filters/readers/proxies etc. to ParaView.
 */

#ifndef vtkPVServerManagerPluginInterface_h
#define vtkPVServerManagerPluginInterface_h

#include "vtkClientServerInterpreterInitializer.h" // for vtkClientServerInterpreterInitializer callback
#include "vtkRemotingCoreModule.h"                 //needed for exports
#include <string>                                  // STL Header
#include <vector>                                  // STL Header

class VTKREMOTINGCORE_EXPORT vtkPVServerManagerPluginInterface
{
public:
  virtual ~vtkPVServerManagerPluginInterface();

  /**
   * Obtain the server-manager configuration xmls, if any.
   */
  virtual void GetXMLs(std::vector<std::string>& vtkNotUsed(xmls)) = 0;

  ///@{
  /**
   * Returns the callback function to call to initialize the interpretor for the
   * new vtk/server-manager classes added by this plugin. Returning nullptr is
   * perfectly valid.
   */
  virtual vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
  GetInitializeInterpreterCallback() = 0;
};
//@}

#endif

// VTK-HeaderTest-Exclude: vtkPVServerManagerPluginInterface.h
