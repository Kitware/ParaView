// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIDirectoryProxy
 *
 * vtkSIDirectoryProxy is the server-implementation for a vtkSMDirectory
 * which will customly handle server file listing for the pull request
 */

#ifndef vtkSIDirectoryProxy_h
#define vtkSIDirectoryProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProxy.h"

class vtkAlgorithmOutput;
class vtkSIProperty;
class vtkPVXMLElement;
class vtkSIProxyDefinitionManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDirectoryProxy : public vtkSIProxy
{
public:
  static vtkSIDirectoryProxy* New();
  vtkTypeMacro(vtkSIDirectoryProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Pull the current state of the underneath implementation
   */
  void Pull(vtkSMMessage* msg) override;

protected:
  vtkSIDirectoryProxy();
  ~vtkSIDirectoryProxy() override;

  // We override it to skip the fake properties (DirectoryList, FileList)
  bool ReadXMLProperty(vtkPVXMLElement* property_element) override;

private:
  vtkSIDirectoryProxy(const vtkSIDirectoryProxy&) = delete;
  void operator=(const vtkSIDirectoryProxy&) = delete;
};

#endif
