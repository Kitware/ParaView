// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIImporterProxy
 *
 * vtkSIImporterProxy is the server-side helper for a vtkSMImporterProxy.
 */

#ifndef vtkSIImporterProxy_h
#define vtkSIImporterProxy_h

#include "vtkRemotingImportModule.h" // for export macro

#include "vtkSIProxy.h"

class VTKREMOTINGIMPORT_EXPORT vtkSIImporterProxy : public vtkSIProxy
{
public:
  static vtkSIImporterProxy* New();
  vtkTypeMacro(vtkSIImporterProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UpdatePipelineInformation() override;

protected:
  vtkSIImporterProxy();
  ~vtkSIImporterProxy() override;

private:
  vtkSIImporterProxy(const vtkSIImporterProxy&) = delete;
  void operator=(const vtkSIImporterProxy&) = delete;
};

#endif
