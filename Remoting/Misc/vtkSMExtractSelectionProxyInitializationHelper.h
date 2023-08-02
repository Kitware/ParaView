// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMExtractSelectionProxyInitializationHelper
 * @brief Initialization helper for the ExtractSelection filter proxy.
 *
 * vtkSMExtractSelectionProxyInitializationHelper initializes the selection
 * input from the selection set on the input source, if it exists.
 */

#ifndef vtkSMExtractSelectionProxyInitializationHelper_h
#define vtkSMExtractSelectionProxyInitializationHelper_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKREMOTINGMISC_EXPORT vtkSMExtractSelectionProxyInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMExtractSelectionProxyInitializationHelper* New();
  vtkTypeMacro(vtkSMExtractSelectionProxyInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMExtractSelectionProxyInitializationHelper();
  ~vtkSMExtractSelectionProxyInitializationHelper() override;

private:
  vtkSMExtractSelectionProxyInitializationHelper(
    const vtkSMExtractSelectionProxyInitializationHelper&) = delete;
  void operator=(const vtkSMExtractSelectionProxyInitializationHelper&) = delete;
};

#endif // vtkSMExtractSelectionProxyInitializationHelper_h
