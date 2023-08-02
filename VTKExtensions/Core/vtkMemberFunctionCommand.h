// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMemberFunctionCommand
 * @brief   Call a class member method in response to a
 * VTK event.
 *
 * vtkMemberFunctionCommand is a vtkCommand-derivative that will listen for VTK
 * events, calling a class member function when a VTK event is received.
 *
 * It is generally more useful than vtkCallbackCommand, which can only call
 * non-member functions in response to a VTK event.
 *
 * Usage: create an instance of vtkMemberFunctionCommand, specialized for the
 * class that will receive events.  Use the SetCallback() method to pass the
 * instance and member function that will be called when an event is received.
 * Use vtkObject::AddObserver() to control which VTK events the
 * vtkMemberFunctionCommand object will receive.
 *
 * Usage:
 *
 * vtkObject* subject = ...
 * foo* observer = ...
 * vtkMemberFunctionCommand<foo>* adapter = vtkMemberFunctionCommand<foo>::New();
 * adapter->SetCallback(observer, &foo::bar);
 * subject->AddObserver(vtkCommand::AnyEvent, adapter);
 *
 * Alternative Usage
 *
 * vtkCommand* command = vtkMakeMemberFunctionCommand(*observer, &foo::Callback);
 * subject->AddObserver(vtkCommand::AnyEvent, command);
 *
 * There are two types of callback methods that could be defined.
 * \li void Callback() -- which takes no arguments.
 * \li void Callback(vtkObject* caller, unsigned long event, void* calldata) --
 * which is passed the same arguments as vtkCommand::Execute(), thus making it
 * possible to get additional details about the event.
 * @sa
 * vtkCallbackCommand
 */

#ifndef vtkMemberFunctionCommand_h
#define vtkMemberFunctionCommand_h

#include "vtkCommand.h"

template <class ClassT>
class vtkMemberFunctionCommand : public vtkCommand
{
  typedef vtkMemberFunctionCommand<ClassT> ThisT;

public:
  typedef vtkCommand Superclass;

  const char* GetClassNameInternal() const override { return "vtkMemberFunctionCommand"; }

  static ThisT* SafeDownCast(vtkObjectBase* o) { return dynamic_cast<ThisT*>(o); }

  static ThisT* New() { return new ThisT(); }

  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  ///@{
  /**
   * Set which class instance and member function will be called when a VTK
   * event is received.
   */
  void SetCallback(ClassT& object, void (ClassT::*method)())
  {
    this->Object = &object;
    this->Method = method;
  }
  ///@}

  void SetCallback(ClassT& object, void (ClassT::*method2)(vtkObject*, unsigned long, void*))
  {
    this->Object = &object;
    this->Method2 = method2;
  }

  void Execute(vtkObject* caller, unsigned long event, void* calldata) override
  {
    if (this->Object && this->Method)
    {
      (this->Object->*this->Method)();
    }
    if (this->Object && this->Method2)
    {
      (this->Object->*this->Method2)(caller, event, calldata);
    }
  }
  void Reset()
  {
    this->Object = nullptr;
    this->Method2 = nullptr;
    this->Method = nullptr;
  }

private:
  vtkMemberFunctionCommand()
  {
    this->Object = nullptr;
    this->Method = nullptr;
    this->Method2 = nullptr;
  }

  ~vtkMemberFunctionCommand() override = default;

  ClassT* Object;
  void (ClassT::*Method)();
  void (ClassT::*Method2)(vtkObject* caller, unsigned long event, void* calldata);

  vtkMemberFunctionCommand(const vtkMemberFunctionCommand&) = delete;
  void operator=(const vtkMemberFunctionCommand&) = delete;
};

/**
 * Convenience function for creating vtkMemberFunctionCommand instances that
 * automatically deduces its arguments.

 * Usage:

 * vtkObject* subject = ...
 * foo* observer = ...
 * vtkCommand* adapter = vtkMakeMemberFunctionCommand(observer, &foo::bar);
 * subject->AddObserver(vtkCommand::AnyEvent, adapter);

 * See Also:
 * vtkMemberFunctionCommand, vtkCallbackCommand
 */

template <class ClassT>
vtkMemberFunctionCommand<ClassT>* vtkMakeMemberFunctionCommand(
  ClassT& object, void (ClassT::*method)())
{
  vtkMemberFunctionCommand<ClassT>* result = vtkMemberFunctionCommand<ClassT>::New();
  result->SetCallback(object, method);
  return result;
}

template <class ClassT>
vtkMemberFunctionCommand<ClassT>* vtkMakeMemberFunctionCommand(
  ClassT& object, void (ClassT::*method)(vtkObject*, unsigned long, void*))
{
  vtkMemberFunctionCommand<ClassT>* result = vtkMemberFunctionCommand<ClassT>::New();
  result->SetCallback(object, method);
  return result;
}

#endif
//-----------------------------------------------------------------------------
// VTK-HeaderTest-Exclude: vtkMemberFunctionCommand.h
