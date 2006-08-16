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
  int InitializeSubInterpretor(int argc, char** argv);

  // Description:
  // This method will make the sub-interpretor represented by this object
  // the active one.
  void MakeCurrent();

protected:
  vtkPVPythonInterpretor();
  ~vtkPVPythonInterpretor();

  // Description:
  // Initialize the interpretor.
  void InitializeInternal();

private:
  vtkPVPythonInterpretor(const vtkPVPythonInterpretor&); // Not implemented.
  void operator=(const vtkPVPythonInterpretor&); // Not implemented.

  vtkPVPythonInterpretorInternal* Internal;
};

#endif

