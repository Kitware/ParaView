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
// .NAME vtkPVDisableStackTraceSignalHandler
// .SECTION Description
// When run on the process it enables a stacktrace signal handler for
// common errors.

#ifndef __vtkPVDisableStackTraceSignalHandler_h
#define __vtkPVDisableStackTraceSignalHandler_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVDisableStackTraceSignalHandler : public vtkPVInformation
{
public:
  static vtkPVDisableStackTraceSignalHandler* New();
  vtkTypeMacro(vtkPVDisableStackTraceSignalHandler, vtkPVInformation);
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
  vtkPVDisableStackTraceSignalHandler(){}
  ~vtkPVDisableStackTraceSignalHandler(){}

private:
  vtkPVDisableStackTraceSignalHandler(const vtkPVDisableStackTraceSignalHandler&); // Not implemented
  void operator=(const vtkPVDisableStackTraceSignalHandler&); // Not implemented
};

#endif
