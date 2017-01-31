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

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVEnableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVEnableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVEnableStackTraceSignalHandler, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*) VTK_OVERRIDE {}

  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE {}
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE {}

protected:
  vtkPVEnableStackTraceSignalHandler() {}
  ~vtkPVEnableStackTraceSignalHandler() {}

private:
  vtkPVEnableStackTraceSignalHandler(const vtkPVEnableStackTraceSignalHandler&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVEnableStackTraceSignalHandler&) VTK_DELETE_FUNCTION;
};

#endif
