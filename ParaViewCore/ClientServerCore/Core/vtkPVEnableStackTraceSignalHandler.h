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
// .NAME vtkPVEnableStackTraceSignalHandler
// .SECTION Description
// When run on the process it enables a stacktrace signal handler for
// common errors.

#ifndef __vtkPVEnableStackTraceSignalHandler_h
#define __vtkPVEnableStackTraceSignalHandler_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVEnableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVEnableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVEnableStackTraceSignalHandler, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*){}

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*){}
  virtual void CopyFromStream(const vtkClientServerStream*){}

protected:
  vtkPVEnableStackTraceSignalHandler(){}
  ~vtkPVEnableStackTraceSignalHandler(){}

private:
  vtkPVEnableStackTraceSignalHandler(const vtkPVEnableStackTraceSignalHandler&); // Not implemented
  void operator=(const vtkPVEnableStackTraceSignalHandler&); // Not implemented
};

#endif
