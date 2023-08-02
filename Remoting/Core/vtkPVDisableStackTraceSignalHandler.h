// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVDisableStackTraceSignalHandler
 *
 * When run on the process it enables a stacktrace signal handler for
 * common errors.
 */

#ifndef vtkPVDisableStackTraceSignalHandler_h
#define vtkPVDisableStackTraceSignalHandler_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTKREMOTINGCORE_EXPORT vtkPVDisableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVDisableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVDisableStackTraceSignalHandler, vtkPVInformation);
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
  vtkPVDisableStackTraceSignalHandler() = default;
  ~vtkPVDisableStackTraceSignalHandler() override = default;

private:
  vtkPVDisableStackTraceSignalHandler(const vtkPVDisableStackTraceSignalHandler&) = delete;
  void operator=(const vtkPVDisableStackTraceSignalHandler&) = delete;
};

#endif
