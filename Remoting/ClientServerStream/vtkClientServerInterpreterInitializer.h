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
/**
 * @class   vtkClientServerInterpreterInitializer
 *
 * vtkClientServerInterpreterInitializer initializes and maintains the global
 * vtkClientServerInterpreter instance for the processes. Use RegisterCallback()
 * to register initialization routines for the interpreter. Use GetInterpreter()
 * to access the interpreter.
 *
 * This class was originally designed to support and maintain multiple
 * interpreter instances. However ParaView no longer has need for that and hence
 * that functionality is no longer made public.
*/

#ifndef vtkClientServerInterpreterInitializer_h
#define vtkClientServerInterpreterInitializer_h

#include "vtkObject.h"
#include "vtkRemotingClientServerStreamModule.h" // Top-level vtkClientServer header.

class vtkClientServerInterpreter;

class VTKREMOTINGCLIENTSERVERSTREAM_EXPORT vtkClientServerInterpreterInitializer : public vtkObject
{
public:
  vtkTypeMacro(vtkClientServerInterpreterInitializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates (and registers) a new interpreter.
   */
  vtkClientServerInterpreter* NewInterpreter();

  /**
   * Get the interpreter for this process. Initializing a new interpreter is
   * expensive. So filters that need to use interpreter temporarily to call
   * methods on a vtkObject can simply use the global interpreter. As a rule,
   * if you need to assign ID's to objects, then you're probably better off
   * creating a new interpreter using NewInterpreter() and using it rather than
   * the global interpreter.
   */
  static vtkClientServerInterpreter* GetGlobalInterpreter();

  /**
   * Provides access to the singleton. This will instantiate
   * vtkClientServerInterpreterInitializer the first time it is called.
   */
  static vtkClientServerInterpreterInitializer* GetInitializer();

  typedef void (*InterpreterInitializationCallback)(vtkClientServerInterpreter*);

  /**
   * Use this method register an interpreter initializer function. Registering
   * such a callback makes it possible to initialize interpreters created in the
   * lifetime of the application, including those that have already been
   * created (but not destroyed). One cannot unregister a callback. The only
   * reason for doing so would be un-loading a plugin, but that's not supported
   * and never will be :).
   */
  void RegisterCallback(InterpreterInitializationCallback callback);

protected:
  static vtkClientServerInterpreterInitializer* New();
  vtkClientServerInterpreterInitializer();
  ~vtkClientServerInterpreterInitializer() override;

  /**
   * Registers an interpreter. This DOES NOT affect the reference count of the
   * interpreter (hence there's no UnRegister).
   */
  void RegisterInterpreter(vtkClientServerInterpreter*);

private:
  vtkClientServerInterpreterInitializer(const vtkClientServerInterpreterInitializer&) = delete;
  void operator=(const vtkClientServerInterpreterInitializer&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
