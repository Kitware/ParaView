/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSource.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// python header first
#include "Python.h"

// self include
#include "pqPythonEventSource.h"

// Qt include
#include <QVariant>
#include <QFile>
#include <QtDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QEvent>

// Qt testing includes
#include "pqObjectNaming.h"


// TODO not have a global instance pointer?
static pqPythonEventSource* Instance = NULL;
static QString PropertyObject;
static QString PropertyResult;
static QWaitCondition WaitResults;

namespace
{
  class pqGetPropertyEvent : public QEvent
    {
  public:
    pqGetPropertyEvent() : QEvent(QEvent::User) {}
    };

}

static PyObject*
QtTesting_playCommand(PyObject* /*self*/, PyObject* args)
{
  // void QtTesting.playCommand('object', 'command', 'arguments')
  //   an exception is thrown in this fails
  
  const char* object = 0;
  const char* command = 0;
  const char* arguments = 0;

  if(!PyArg_ParseTuple(args, const_cast<char*>("sss"), &object, &command, &arguments))
    {
    PyErr_SetString(PyExc_TypeError, "bad arguments to playCommand()");
    return NULL;
    }

  if(Instance)
    {
    Instance->postNextEvent(object, command, arguments);
    }
  else
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  return Py_BuildValue(const_cast<char*>(""));
}

static PyObject*
QtTesting_getProperty(PyObject* /*self*/, PyObject* args)
{
  // string QtTesting.getProperty('object', 'property')
  //    returns the string value of the property
  
  const char* object = 0;
  const char* property = 0;

  if(!PyArg_ParseTuple(args, const_cast<char*>("ss"), &object, &property))
    {
    return NULL;
    }

  if(!Instance)
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  PropertyResult = property;
  PropertyObject = object;
  
  QCoreApplication::postEvent(Instance, new pqGetPropertyEvent());
  QMutex mut;
  mut.lock();
  WaitResults.wait(&mut);
    
  if(PropertyObject == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }

  return Py_BuildValue(const_cast<char*>("s"), 
             PropertyResult.toAscii().data());
}

static PyMethodDef QtTestingMethods[] = {
  {
    const_cast<char*>("playCommand"), 
    QtTesting_playCommand,
    METH_VARARGS,
    const_cast<char*>("Play a test command.")
  },
  {
    const_cast<char*>("getProperty"),
    QtTesting_getProperty,
    METH_VARARGS,
    const_cast<char*>("Get a property of an object.")
  },

  {NULL, NULL, 0, NULL} // Sentinal
};

PyMODINIT_FUNC
initQtTesting(void)
{
  Py_InitModule(const_cast<char*>("QtTesting"), QtTestingMethods);
}


class pqPythonEventSource::pqInternal
{
public:
  QString FileName;
  PyThreadState* MainThreadState;
};

pqPythonEventSource::pqPythonEventSource(QObject* p)
  : pqThreadedEventSource(p)
{
  this->Internal = new pqInternal;
  
  // initialize python
  Py_Initialize();

  // initialize the QtTesting module
  initQtTesting();

  // initialize threading
  PyEval_InitThreads();
  this->Internal->MainThreadState = PyThreadState_Get();
  PyEval_ReleaseLock();
}

pqPythonEventSource::~pqPythonEventSource()
{
  delete this->Internal;
}

void pqPythonEventSource::setContent(const QString& path)
{
  // start the python thread
  this->Internal->FileName = path;
  this->start();
}
  
bool pqPythonEventSource::event(QEvent* e)
{
  if(dynamic_cast<pqGetPropertyEvent*>(e))
    {
    QObject* qobject = pqObjectNaming::GetObject(PropertyObject);
    if(!qobject)
      {
      PropertyObject = QString::null;
      }
    else
      {
      PropertyResult = qobject->property(
        PropertyResult.toAscii().data()).toString();
      }
    WaitResults.wakeAll();
    return true;
    }
  return pqThreadedEventSource::event(e);
}

void pqPythonEventSource::run()
{

  QString filename = this->Internal->FileName;
  FILE* pythonScript = fopen(filename.toAscii().data(), "r");
  if(!pythonScript)
    {
    printf("Unable to open python script\n");
    return;
    }
  
  PyEval_AcquireLock();
  PyInterpreterState* mainInterpreterState;
  mainInterpreterState = this->Internal->MainThreadState->interp;
  PyThreadState* myThreadState = PyThreadState_New(mainInterpreterState);
  PyThreadState_Swap(myThreadState);
  
  Instance = this;

  // finally run the script
  int result = PyRun_SimpleFile(pythonScript, filename.toAscii().data());
  if( result == 2 )
    {
    printf("invalid python file\n");
    }
  
  PyThreadState_Swap(NULL);

  PyThreadState_Clear(myThreadState);
  PyThreadState_Delete(myThreadState);
  
  PyEval_ReleaseLock();
  
  fclose(pythonScript);

  this->done(result);
}

