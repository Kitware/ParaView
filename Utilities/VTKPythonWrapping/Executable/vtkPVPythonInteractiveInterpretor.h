/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInteractiveInterpretor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonInteractiveInterpretor - interpretor for interactive shells.
// .SECTION Description
// vtkPVPythonInteractiveInterpretor is vtkPVPythonInterpretor subclass designed
// to be used by interactive shells. It mimicks the behaviour of the interactive
// console (much like the default python shell) providing the "read-eval-print"
// loops. It also handles incomplete statements correctly. It uses "code"
// module provided by python to achieve this.

#ifndef __vtkPVPythonInteractiveInterpretor_h
#define __vtkPVPythonInteractiveInterpretor_h

#include "vtkPVPythonInterpretor.h"

class VTK_EXPORT vtkPVPythonInteractiveInterpretor : public vtkPVPythonInterpretor
{
public:
  static vtkPVPythonInteractiveInterpretor* New();
  vtkTypeMacro(vtkPVPythonInteractiveInterpretor, vtkPVPythonInterpretor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Push a line of code. It should have have trailing newlines. It can have
  // internal newlines. This can accept incomplete input. A command is executed
  // only after the complete input is received.
  // Look at python module documentation for code.InteractiveConsole.push() 
  // for further details.
  // The return value is True if more input is required, False if the line 
  // was dealt with in some way.
  bool Push(const char* const code);

  // Description:
  // Clears all previously pushed unhandled input.
  // Look at python module documentation for code.InteractiveConsole.resetbuffer() 
  // for further details.
  void ResetBuffer();

//BTX
protected:
  vtkPVPythonInteractiveInterpretor();
  ~vtkPVPythonInteractiveInterpretor();

  // Description:
  // Initialize the interpretor.
  virtual void InitializeInternal();
private:
  vtkPVPythonInteractiveInterpretor(const vtkPVPythonInteractiveInterpretor&); // Not implemented
  void operator=(const vtkPVPythonInteractiveInterpretor&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

