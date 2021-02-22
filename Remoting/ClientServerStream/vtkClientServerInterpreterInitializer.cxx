/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerInterpreterInitializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerInterpreterInitializer.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <vector>

class vtkClientServerInterpreterInitializer::vtkInternals
{
public:
  typedef std::vector<vtkWeakPointer<vtkClientServerInterpreter> > VectorOfInterpreters;
  VectorOfInterpreters Interpreters;
  typedef std::vector<vtkClientServerInterpreterInitializer::InterpreterInitializationCallback>
    VectorOfCallbacks;
  VectorOfCallbacks Callbacks;
};

//----------------------------------------------------------------------------
// Can't use vtkStandardNewMacro since it adds the instantiator function which
// does not compile since vtkClientServerInterpreterInitializer::New() is
// protected.
vtkClientServerInterpreterInitializer* vtkClientServerInterpreterInitializer::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkClientServerInterpreterInitializer");
  if (ret)
  {
    return static_cast<vtkClientServerInterpreterInitializer*>(ret);
  }
  vtkClientServerInterpreterInitializer* o = new vtkClientServerInterpreterInitializer;
  o->InitializeObjectBase();
  return o;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreterInitializer::vtkClientServerInterpreterInitializer()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkClientServerInterpreterInitializer::~vtkClientServerInterpreterInitializer()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreterInitializer* vtkClientServerInterpreterInitializer::GetInitializer()
{
  static vtkSmartPointer<vtkClientServerInterpreterInitializer> Singleton;
  if (!Singleton)
  {
    Singleton.TakeReference(vtkClientServerInterpreterInitializer::New());
  }
  return Singleton;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter* vtkClientServerInterpreterInitializer::GetGlobalInterpreter()
{
  static vtkSmartPointer<vtkClientServerInterpreter> Singleton;
  if (!Singleton)
  {
    vtkClientServerInterpreterInitializer* initializer =
      vtkClientServerInterpreterInitializer::GetInitializer();
    Singleton.TakeReference(initializer->NewInterpreter());
  }
  return Singleton;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter* vtkClientServerInterpreterInitializer::NewInterpreter()
{
  vtkClientServerInterpreter* interp = vtkClientServerInterpreter::New();
  // THIS DOES NOT AFFECT REF-COUNT.
  this->RegisterInterpreter(interp);
  return interp;
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreterInitializer::RegisterInterpreter(vtkClientServerInterpreter* interp)
{
  // THIS DOES NOT AFFECT REF-COUNT.
  this->Internals->Interpreters.push_back(interp);

  // Initialize using existing callbacks.
  vtkInternals::VectorOfCallbacks::iterator iter;
  for (iter = this->Internals->Callbacks.begin(); iter != this->Internals->Callbacks.end(); ++iter)
  {
    (*(*iter))(interp);
  }
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreterInitializer::RegisterCallback(
  vtkClientServerInterpreterInitializer::InterpreterInitializationCallback callback)
{
  this->Internals->Callbacks.push_back(callback);

  vtkInternals::VectorOfInterpreters::iterator iter;
  for (iter = this->Internals->Interpreters.begin(); iter != this->Internals->Interpreters.end();
       ++iter)
  {
    if (iter->GetPointer() != nullptr)
    {
      (*callback)(iter->GetPointer());
    }
  }
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreterInitializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
