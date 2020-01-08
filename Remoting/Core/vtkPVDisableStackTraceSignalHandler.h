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
  vtkPVDisableStackTraceSignalHandler() {}
  ~vtkPVDisableStackTraceSignalHandler() override {}

private:
  vtkPVDisableStackTraceSignalHandler(const vtkPVDisableStackTraceSignalHandler&) = delete;
  void operator=(const vtkPVDisableStackTraceSignalHandler&) = delete;
};

#endif
