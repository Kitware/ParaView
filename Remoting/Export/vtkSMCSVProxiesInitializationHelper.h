// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMCSVProxiesInitializationHelper
 * @brief   initialization helper for
 * (writers, PCSVReader) proxy.
 *
 * vtkSMCSVProxiesInitializationHelper is an initialization helper for
 * the PCSVReader or PCSVWriter proxy that sets up the delimiter to use based on the
 * file extension. If the file extension is .txt or .tsv then \verbatim'\t'\endverbatim is set as
 * the delimiter.
 */

#ifndef vtkSMCSVProxiesInitializationHelper_h
#define vtkSMCSVProxiesInitializationHelper_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKREMOTINGEXPORT_EXPORT vtkSMCSVProxiesInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMCSVProxiesInitializationHelper* New();
  vtkTypeMacro(vtkSMCSVProxiesInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMCSVProxiesInitializationHelper();
  ~vtkSMCSVProxiesInitializationHelper() override;

private:
  vtkSMCSVProxiesInitializationHelper(const vtkSMCSVProxiesInitializationHelper&) = delete;
  void operator=(const vtkSMCSVProxiesInitializationHelper&) = delete;
};

#endif
