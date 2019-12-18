/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  vtkPVEnableStackTraceSignalHandler() {}
  ~vtkPVEnableStackTraceSignalHandler() override {}

private:
  vtkPVEnableStackTraceSignalHandler(const vtkPVEnableStackTraceSignalHandler&) = delete;
  void operator=(const vtkPVEnableStackTraceSignalHandler&) = delete;
};

#endif
