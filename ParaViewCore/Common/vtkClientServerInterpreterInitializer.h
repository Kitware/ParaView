/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerInterpreterInitializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientServerInterpreterInitializer
// .SECTION Description
// vtkClientServerInterpreterInitializer initializes and maintains the global
// vtkClientServerInterpreter instance for the processes. Use RegisterCallback()
// to register initialization routines for the interpreter. Use GetInterpreter()
// to access the interpreter.
//
// This class was originally designed to support and maintain multiple
// interpreter instances. However ParaView no longer has need for that and hence
// that functionality is no longer made public.

#ifndef __vtkClientServerInterpreterInitializer_h
#define __vtkClientServerInterpreterInitializer_h

#include "vtkObject.h"

class vtkClientServerInterpreter;

class VTK_EXPORT vtkClientServerInterpreterInitializer : public vtkObject
{
public:
  vtkTypeMacro(vtkClientServerInterpreterInitializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the interpreter for this process.
  static vtkClientServerInterpreter* GetInterpreter();

  // Description
  // Provides access to the singleton. This will instantiate
  // vtkClientServerInterpreterInitializer the first time it is called.
  static vtkClientServerInterpreterInitializer* GetInitializer();


//BTX
  typedef void (*InterpreterInitializationCallback)(vtkClientServerInterpreter*);

  // Description:
  // Use this method register an interpreter initializer function. Registering
  // such a callback makes it possible to initialize interpreters created in the
  // lifetime of the application, including those that have already been
  // created (but not destroyed). One cannot unregister a callback. The only
  // reason for doing so would be un-loading a plugin, but that's not supported
  // and never will be :).
  void RegisterCallback(InterpreterInitializationCallback callback);

protected:
  static vtkClientServerInterpreterInitializer* New();
  vtkClientServerInterpreterInitializer();
  ~vtkClientServerInterpreterInitializer();

  // Description:
  // Creates (and registers) a new interpreter.
  vtkClientServerInterpreter* NewInterpreter();

  // Description:
  // Registers an interpreter. This DOES NOT affect the reference count of the
  // interpreter (hence there's no UnRegister).
  void RegisterInterpreter(vtkClientServerInterpreter*);

private:
  vtkClientServerInterpreterInitializer(const vtkClientServerInterpreterInitializer&); // Not implemented
  void operator=(const vtkClientServerInterpreterInitializer&); // Not implemented

  class vtkInternals;
  vtkInternals *Internals;
//ETX
};

#endif
