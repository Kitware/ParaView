// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMImporterProxy
 * @brief   proxy for vtkImporter classes.
 *
 * vtkSMImporterProxy provides methods to import scene from a file in client-server
 * or builtin sessions.
 */
#ifndef vtkSMImporterProxy_h
#define vtkSMImporterProxy_h

#include "vtkRemotingImportModule.h" // for export macro

#include "vtkAOSDataArrayTemplate.h" // for arg
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <memory> // for unique_ptr

class vtkDataAssembly;
class vtkPVXMLElement;
class vtkSMSessionProxyManager;
class vtkSMRenderViewProxy;

class VTKREMOTINGIMPORT_EXPORT vtkSMImporterProxy : public vtkSMProxy
{
public:
  static vtkSMImporterProxy* New();
  vtkTypeMacro(vtkSMImporterProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Imports scene from the file.
   */
  void Import(vtkSMRenderViewProxy* renderView);

  ///@{
  /**
   * Returns the suggested file extensions for this importer.
   */
  const std::vector<std::string>& GetFileExtensions() const;
  ///@}

  void UpdatePipelineInformation() override;

protected:
  vtkSMImporterProxy();
  ~vtkSMImporterProxy() override;

  /**
   * Read attributes from an XML element.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pxm, vtkPVXMLElement* element) override;

private:
  vtkSMImporterProxy(const vtkSMImporterProxy&) = delete;
  void operator=(const vtkSMImporterProxy&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
