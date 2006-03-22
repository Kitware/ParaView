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
    
    PyThreadState_Swap(Interpreter);

/*
    PyObject* const path = PySys_GetObject("path");
    PyList_Insert(path, 0, PyString_FromString(VTK_PYTHON_LIBRARY_DIR));
    PyList_Insert(path, 0, PyString_FromString(VTK_PYTHON_PACKAGE_DIR));
*/

/*    
    char tmpPath[5];
    sprintf(tmpPath,"path");
    PyObject* path = PySys_GetObject(tmpPath);
    PyObject* newpath;
    if ( vtksys::SystemTools::FileExists(VTK_PYTHON_LIBRARY_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_LIBRARY_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }
    if ( vtksys::SystemTools::FileExists(VTK_PYTHON_PACKAGE_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_PACKAGE_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }
*/

    PyThreadState_Swap(0);
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
