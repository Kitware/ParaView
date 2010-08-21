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
// .NAME vtkClientServerInterpreterInitializer
// .SECTION Description
// vtkClientServerInterpreter instances need to initialized before they can be used.
// Also as and when new plugins are loaded, we need to update the already
// existing instances of the vtkClientServerInterpreter with the new
// information. To streamline this process, we have singleton
// vtkClientServerInterpreterInitializer. One simply registers an interpretor
// with this class or use NewInterpreter() to create a new interpreter and
// that's it. It will be initialized and updated automatically.

#ifndef __vtkClientServerInterpreterInitializer_h
#define __vtkClientServerInterpreterInitializer_h

#include "vtkObject.h"

class vtkClientServerInterpreter;

class VTK_EXPORT vtkClientServerInterpreterInitializer : public vtkObject
{
public:
  vtkTypeMacro(vtkClientServerInterpreterInitializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Provides access to the singleton. This will instantiate
  // vtkClientServerInterpreterInitializer the first time it is called.
  static vtkClientServerInterpreterInitializer* GetInitializer();

  // Description:
  // Creates (and registers) a new interpreter.
  vtkClientServerInterpreter* NewInterpreter();

  // Description:
  // Registers an interpreter. This DOES NOT affect the reference count of the
  // interpreter (hence there's no UnRegister).
  void RegisterInterpreter(vtkClientServerInterpreter*);

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

private:
  vtkClientServerInterpreterInitializer(const vtkClientServerInterpreterInitializer&); // Not implemented
  void operator=(const vtkClientServerInterpreterInitializer&); // Not implemented

  class vtkInternals;
  vtkInternals *Internals;
//ETX
};

#endif
