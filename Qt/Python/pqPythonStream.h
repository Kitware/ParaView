/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPythonStream_h
#define _pqPythonStream_h

#include <QObject>

////////////////////////////////////////////////////////////////////////////////////////////
// pqPythonStream

/// Helper-class that converts Python stream operations into Qt signals
class pqPythonStream :
  public QObject
{
  Q_OBJECT

public:
  pqPythonStream();
  
  void write(const QString&);
  
signals:
  void streamWrite(const QString&);  

private:
  pqPythonStream(const pqPythonStream&);
  pqPythonStream& operator=(const pqPythonStream&);
};

/// Wraps a pqPythonStream in a Python object (the return-type is void* instead of PyObject* due to conflicts between Qt and the Python headers)
void* pqWrap(pqPythonStream&);

#endif // !_pqPythonStream_h

