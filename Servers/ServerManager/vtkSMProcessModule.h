/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProcessModule
// .SECTION Description

#ifndef __vtkSMProcessModule_h
#define __vtkSMProcessModule_h

#include "vtkObject.h"

class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkCallbackCommand;

class VTK_EXPORT vtkSMProcessModule : public vtkObject
{
public:
  static vtkSMProcessModule* New();
  vtkTypeRevisionMacro(vtkSMProcessModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize/Finalize the process module's
  // vtkClientServerInterpreter.
  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();

  // Description:
  void ProcessStream(vtkClientServerStream* stream);

  // Description:
  void ClearLastResult();

protected:
  vtkSMProcessModule();
  ~vtkSMProcessModule();

  vtkClientServerInterpreter* Interpreter;

  static void InterpreterCallbackFunction(vtkObject* caller,
                                          unsigned long eid,
                                          void* cd, void* d);
  virtual void InterpreterCallback(unsigned long eid, void*);

  vtkCallbackCommand* InterpreterObserver;
  int ReportInterpreterErrors;

private:
  vtkSMProcessModule(const vtkSMProcessModule&); // Not implemented
  void operator=(const vtkSMProcessModule&); // Not implemented
};

#endif
