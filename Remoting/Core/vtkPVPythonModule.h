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

/**
 * @class   vtkPVPythonModule
 * @brief   Stores code and option for python modules
 *
 *
 *
 * vtkPVPythonModule is a simple class that stores some Python source code that
 * makes up a Python module as well as some state variables about the module
 * (such as its name).
 *
*/

#ifndef vtkPVPythonModule_h
#define vtkPVPythonModule_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVPythonModule : public vtkObject
{
public:
  vtkTypeMacro(vtkPVPythonModule, vtkObject);
  static vtkPVPythonModule* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the full Python source for this module.
   */
  vtkGetStringMacro(Source);
  vtkSetStringMacro(Source);
  //@}

  //@{
  /**
   * Set/get the full name for this module (e.g. package.subpackage.module).
   */
  vtkGetStringMacro(FullName);
  vtkSetStringMacro(FullName);
  //@}

  //@{
  /**
   * Set/get a flag indicating whether this module is actually a package (which
   * can have submodules).
   */
  vtkGetMacro(IsPackage, int);
  vtkSetMacro(IsPackage, int);
  vtkBooleanMacro(IsPackage, int);
  //@}

  /**
   * Register the Python module.  Once registered, the module can be retrieved
   * with GetModule and HaveModule.  Python interpreters can query these
   * global methods in a custom import mechanism.
   */
  static void RegisterModule(vtkPVPythonModule* module);

  /**
   * Return the registered Python module with the given full module name.  If
   * no such module has been registered, this returns nullptr.
   */
  static vtkPVPythonModule* GetModule(const char* fullname);

  /**
   * Returns 1 if a Python module with the given full name has been registered,
   * 0 otherwise.
   */
  static int HasModule(const char* fullname)
  {
    return (vtkPVPythonModule::GetModule(fullname) != nullptr);
  }

protected:
  vtkPVPythonModule();
  ~vtkPVPythonModule() override;

  char* Source;
  char* FullName;
  int IsPackage;

private:
  vtkPVPythonModule(const vtkPVPythonModule&) = delete;
  void operator=(const vtkPVPythonModule&) = delete;
};

#endif // vtkPVPythonModule_h
