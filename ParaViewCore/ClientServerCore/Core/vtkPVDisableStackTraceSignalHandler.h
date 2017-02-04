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

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVDisableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVDisableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVDisableStackTraceSignalHandler, vtkPVInformation);
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
  vtkPVDisableStackTraceSignalHandler() {}
  ~vtkPVDisableStackTraceSignalHandler() {}

private:
  vtkPVDisableStackTraceSignalHandler(
    const vtkPVDisableStackTraceSignalHandler&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVDisableStackTraceSignalHandler&) VTK_DELETE_FUNCTION;
};

#endif
