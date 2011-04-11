/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPythonModule.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkPVPythonModule - Stores code and option for python modules
//
// .SECTION Description
//
// vtkPVPythonModule is a simple class that stores some Python source code that
// makes up a Python module as well as some state variables about the module
// (such as its name).
//

#ifndef __vtkPVPythonModule_h
#define __vtkPVPythonModule_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVPythonModule : public vtkObject
{
public:
  vtkTypeMacro(vtkPVPythonModule, vtkObject);
  static vtkPVPythonModule *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/get the full Python source for this module.
  vtkGetStringMacro(Source);
  vtkSetStringMacro(Source);

  // Description:
  // Set/get the full name for this module (e.g. package.subpackage.module).
  vtkGetStringMacro(FullName);
  vtkSetStringMacro(FullName);

  // Description:
  // Set/get a flag indicating whether this module is actually a package (which
  // can have submodules).
  vtkGetMacro(IsPackage, int);
  vtkSetMacro(IsPackage, int);
  vtkBooleanMacro(IsPackage, int);

  // Description:
  // Register the Python module.  Once registered, the module can be retrieved
  // with GetModule and HaveModule.  Python interpreters can query these
  // global methods in a custom import mechanism.
  static void RegisterModule(vtkPVPythonModule *module);

  // Description:
  // Return the registered Python module with the given full module name.  If
  // no such module has been registered, this returns NULL.
  static vtkPVPythonModule *GetModule(const char *fullname);

  // Description:
  // Returns 1 if a Python module with the given full name has been registered,
  // 0 otherwise.
  static int HasModule(const char *fullname) {
    return (vtkPVPythonModule::GetModule(fullname) != NULL);
  }

protected:
  vtkPVPythonModule();
  ~vtkPVPythonModule();

  char *Source;
  char *FullName;
  int   IsPackage;

private:
  vtkPVPythonModule(const vtkPVPythonModule &); // Not implemented
  void operator=(const vtkPVPythonModule &);    // Not implemented
};

#endif //__vtkPVPythonModule_h
