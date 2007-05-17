/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInterpretor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonInterpretor - Encapsulates a single instance of a python
// interpretor.
// .SECTION Description
// Encapsulates a python interpretor. It also initializes the interpretor
// with the paths to the paraview libraries and modules. This object can 
// represent the main interpretor or any sub-interpretor.

#ifndef __vtkPVPythonInterpretor_h
#define __vtkPVPythonInterpretor_h

#include "vtkObject.h"

class vtkPVPythonInterpretorInternal;
class VTK_EXPORT vtkPVPythonInterpretor : public vtkObject
{
public:
  static vtkPVPythonInterpretor* New();
  vtkTypeRevisionMacro(vtkPVPythonInterpretor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes python and starts the interprestor's event loop
  // i.e. call // Py_Main().
  int PyMain(int argc, char** argv);
  
  // Description:
  // Initializes python and create a sub-interpretor context.
  // Subinterpretors don't get argc/argv, however, argv[0] is
  // used to set the executable path if not already set. The
  // executable path is used to locate paraview python modules.
  // This method calls ReleaseControl() at the end of the call, hence the newly
  // created interpretor is not the active interpretor when the control returns.
  // Use MakeCurrent() to make it active.
  int InitializeSubInterpretor(int argc, char** argv);

  // Description:
  // This method will make the sub-interpretor represented by this object
  // the active one. If MultithreadSupport is enabled, this method also acquires
  // the global interpretor lock. A MakeCurrent() call must have a corresponding
  // ReleaseControl().
  void MakeCurrent();

  // Description:
  // Helper function that executes a script using PyRun_SimpleString() - handles
  // some pesky details with DOS line endings. */
  // This method calls MakeCurrent() at the start and ReleaseControl() at the
  // end, hence the interpretor will not be the active one when the control
  // returns from this call.
  void RunSimpleString(const char* const script);

  // Description:
  // Call in a subinterpretter to pause it and return control to the 
  // main interpretor. If MultithreadSupport is enabled, this method also
  // releases the global interpretor lock.
  void ReleaseControl();

  // Description:
  // When the interpretor is used in a multithreaded environment, python
  // requires additional initialization/locking. Set this to true to enable
  // initialization and locking of the global interpretor, if application is
  // multithreaded with sub-interpretors being initialized by different threads.
  // Changing this flag after InitializeSubInterpretor() has no effect.
  vtkSetMacro(MultithreadSupport, bool);
  vtkGetMacro(MultithreadSupport, bool);

protected:
  vtkPVPythonInterpretor();
  ~vtkPVPythonInterpretor();

  // Description:
  // Initialize the interpretor.
  void InitializeInternal();

  char* ExecutablePath;
  vtkSetStringMacro(ExecutablePath);

  bool MultithreadSupport;

private:
  vtkPVPythonInterpretor(const vtkPVPythonInterpretor&); // Not implemented.
  void operator=(const vtkPVPythonInterpretor&); // Not implemented.

  vtkPVPythonInterpretorInternal* Internal;
};

#endif

