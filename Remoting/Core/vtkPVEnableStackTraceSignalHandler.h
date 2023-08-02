// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVEnableStackTraceSignalHandler
 *
 * When run on the process it enables a stacktrace signal handler for
 * common errors.
 */

#ifndef vtkPVEnableStackTraceSignalHandler_h
#define vtkPVEnableStackTraceSignalHandler_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTKREMOTINGCORE_EXPORT vtkPVEnableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVEnableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVEnableStackTraceSignalHandler, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override {}

  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override {}
  void CopyFromStream(const vtkClientServerStream*) override {}

protected:
  vtkPVEnableStackTraceSignalHandler() = default;
  ~vtkPVEnableStackTraceSignalHandler() override = default;

private:
  vtkPVEnableStackTraceSignalHandler(const vtkPVEnableStackTraceSignalHandler&) = delete;
  void operator=(const vtkPVEnableStackTraceSignalHandler&) = delete;
};

#endif
