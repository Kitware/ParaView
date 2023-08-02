// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPLYWriterProxyInitializationHelper
 * @brief   initialization helper for
 * (writers, PPLYWriter) proxy.
 *
 * vtkSMPLYWriterProxyInitializationHelper is an initialization helper for
 * the PPLYWriter proxy that sets up the "ColorArrayName" and "LookupTable"
 * using the coloring state in the active view.
 */

#ifndef vtkSMPLYWriterProxyInitializationHelper_h
#define vtkSMPLYWriterProxyInitializationHelper_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKREMOTINGMISC_EXPORT vtkSMPLYWriterProxyInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMPLYWriterProxyInitializationHelper* New();
  vtkTypeMacro(vtkSMPLYWriterProxyInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMPLYWriterProxyInitializationHelper();
  ~vtkSMPLYWriterProxyInitializationHelper() override;

private:
  vtkSMPLYWriterProxyInitializationHelper(const vtkSMPLYWriterProxyInitializationHelper&) = delete;
  void operator=(const vtkSMPLYWriterProxyInitializationHelper&) = delete;
};

#endif
