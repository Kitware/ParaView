/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqPythonInterpreter.h"

#include <vtkPython.h>

////////////////////////////////////////////////////////////////////////
// pqPythonInterpreter::pqImplementation

class pqPythonInterpreter::pqImplementation
{
public:
  pqImplementation() :
    Interpreter(0)
  {
    if(!Py_IsInitialized())
      Py_Initialize();
      
    Interpreter = Py_NewInterpreter();
  }
  
  ~pqImplementation()
  {
    PyThreadState_Swap(Interpreter);
    Py_EndInterpreter(Interpreter);
    PyThreadState_Swap(0);
  }
 
  void MakeCurrent()
  {
    PyThreadState_Swap(Interpreter);
  }
 
private:
  PyThreadState* Interpreter;
};

////////////////////////////////////////////////////////////////////////
// pqPythonInterpreter

pqPythonInterpreter::pqPythonInterpreter() :
  Implementation(new pqImplementation())
{
}

pqPythonInterpreter::~pqPythonInterpreter()
{
  delete Implementation;
}

void pqPythonInterpreter::MakeCurrent()
{
  Implementation->MakeCurrent();
}
