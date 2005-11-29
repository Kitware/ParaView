/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPythonInterpreter_h
#define _pqPythonInterpreter_h

#include "pqPythonExport.h"

/// Encapsulates a single instance of a Python interpreter.
/** Create an instance of pqPythonInterpreter and call its MakeCurrent() method
    before making any other Python calls.  You may create multiple interpreter
    instances (separate Python runtime environments) and execute python calls in
    each by calling MakeCurrent() to "switch" between interpreters */
class PQPYTHON_EXPORT pqPythonInterpreter
{
public:
  pqPythonInterpreter();
  ~pqPythonInterpreter();
  
  /// Makes this instance the "current" instance, for subsequent Python calls
  void MakeCurrent();
  
private:
  pqPythonInterpreter(const pqPythonInterpreter&);
  pqPythonInterpreter& operator=(const pqPythonInterpreter&);
  
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqPythonInterpreter_h
