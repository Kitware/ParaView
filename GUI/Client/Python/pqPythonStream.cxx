/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqPythonStream.h"

#undef slots
#include <vtkPython.h>

////////////////////////////////////////////////////////////////////////////////////////////
// pqPythonStream

pqPythonStream::pqPythonStream()
{
}

void pqPythonStream::write(const QString& String)
{
  emit streamWrite(String);
}

////////////////////////////////////////////////////////////////////////////////////////////
// pqPythonStreamWrapper

namespace
{

struct pqPythonStreamWrapper
{
  PyObject_HEAD
  pqPythonStream* stream;
};

pqPythonStream* getStream(PyObject*);

  PyObject* pqPythonStreamWrapperNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
  PyObject* const self = type->tp_alloc(type, 0);
  if(self)
    reinterpret_cast<pqPythonStreamWrapper*>(self)->stream = 0;
  return self;
}

PyObject* pqPythonStreamWrapperWrite(PyObject* self, PyObject* args)
{
  pqPythonStream* stream = getStream(self);
  if(!stream)
    return 0;
    
  const char* string;
  if(!PyArg_ParseTuple(args, const_cast<char*>("s"), &string))
    return 0;
    
  stream->write(string);
  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef pqPythonStreamWrapperMethods[] =
{
  { const_cast<char*>("write"), pqPythonStreamWrapperWrite, METH_VARARGS, const_cast<char*>("Write to the stream") },
  { 0 }
};

static PyTypeObject pqPythonStreamWrapperType = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    const_cast<char*>("pqPythonStreamWrapper"),   // tp_name
    sizeof(pqPythonStreamWrapper), // tp_basicsize
    0,                         // tp_itemsize
    0,                         // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    0,                         // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash 
    0,                         // tp_call
    0,                         // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    const_cast<char*>("pqPythonStreamWrapper"),   //  tp_doc 
    0,                         //  tp_traverse 
    0,                         //  tp_clear 
    0,                         //  tp_richcompare 
    0,                         //  tp_weaklistoffset 
    0,                         //  tp_iter 
    0,                         //  tp_iternext 
    pqPythonStreamWrapperMethods, //  tp_methods 
    0,                         //  tp_members 
    0,                         //  tp_getset 
    0,                         //  tp_base 
    0,                         //  tp_dict 
    0,                         //  tp_descr_get 
    0,                         //  tp_descr_set 
    0,                         //  tp_dictoffset 
    0,                         //  tp_init 
    0,                         //  tp_alloc 
    pqPythonStreamWrapperNew,  //  tp_new 
};

pqPythonStream* getStream(PyObject* Object)
{
  if(!Object)
    return 0;
  if(!PyObject_TypeCheck(Object, &pqPythonStreamWrapperType))
    return 0;
  return reinterpret_cast<pqPythonStreamWrapper*>(Object)->stream;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////
// pqWrap

void* pqWrap(pqPythonStream& Stream)
{
  if(PyType_Ready(&pqPythonStreamWrapperType) < 0)
    return 0;
        
  pqPythonStreamWrapper* const wrapper = PyObject_New(pqPythonStreamWrapper, &pqPythonStreamWrapperType);
  if(wrapper)
    wrapper->stream = &Stream;
    
  return reinterpret_cast<PyObject*>(wrapper);
}

