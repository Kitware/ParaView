/*=========================================================================

   Program: ParaView
   Module:    pqPythonEventSource.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
// TODO:  Fix this so we don't depend on VTK
#include "vtkPython.h"
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC extern "C" void
#endif // PyMODINIT_FUNC

// self include
#include "pqPythonEventSource.h"

// system includes
#include <signal.h>

// Qt include
#include <QVariant>
#include <QFile>
#include <QtDebug>
#include <QCoreApplication>
#include <QEvent>
#include <QStringList>
#include <QThread>
#include <QApplication>
#include <QMetaObject>
#include <QMetaProperty>

// Qt testing includes
#include "pqObjectNaming.h"
#include "pqWidgetEventPlayer.h"
#include "pqEventDispatcher.h"


// TODO not have a global instance pointer?
static pqPythonEventSource* Instance = NULL;
static QString PropertyObject;
static QString PropertyResult;
static QString PropertyValue;
static QStringList ObjectList;


static PyObject*
QtTesting_playCommand(PyObject* /*self*/, PyObject* args)
{
  // this gives the main thread some time to refresh before we start sending
  // commands. Avoids crazy hangs in QDialog::exec() calls.
  pqThreadedEventSource::msleep(500);

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
    if(!Instance->postNextEvent(object, command, arguments))
      {
      PyErr_SetString(PyExc_AssertionError, "error processing event");
      return NULL;
      }
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

  PropertyObject = object;
  PropertyResult = property;
  PropertyValue = QString::null;

  if(Instance && QThread::currentThread() != QApplication::instance()->thread())
    {
    QMetaObject::invokeMethod(Instance, "threadGetProperty", Qt::QueuedConnection);
    if(!Instance->waitForGUI())
      {
      PyErr_SetString(PyExc_ValueError, "error getting property");
      return NULL;
      }
    }
  else if(QThread::currentThread() == QApplication::instance()->thread())
    {
    PropertyValue = pqPythonEventSource::getProperty(PropertyObject, PropertyResult);
    }
  else
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  if(PropertyObject == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }
  
  if(PropertyResult == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "property not found");
    return NULL;
    }

  return Py_BuildValue(const_cast<char*>("s"), 
             PropertyValue.toAscii().data());
}

static PyObject*
QtTesting_setProperty(PyObject* /*self*/, PyObject* args)
{
  // string QtTesting.setProperty('object', 'property', 'value')
  //    returns the string value of the property
  
  const char* object = 0;
  const char* property = 0;
  const char* value = 0;

  if(!PyArg_ParseTuple(args, const_cast<char*>("sss"), &object, 
                                                      &property, &value))
    {
    return NULL;
    }

  PropertyObject = object;
  PropertyResult = property;
  PropertyValue = value;

  if(Instance && QThread::currentThread() != QApplication::instance()->thread())
    {
    QMetaObject::invokeMethod(Instance, "threadSetProperty", Qt::QueuedConnection);
    if(!Instance->waitForGUI())
      {
      PyErr_SetString(PyExc_ValueError, "error setting property");
      return NULL;
      }
    }
  else if(QThread::currentThread() == QApplication::instance()->thread())
    {
    pqPythonEventSource::setProperty(PropertyObject, 
                                     PropertyResult,
                                     PropertyValue);
    }
  else
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  if(PropertyObject == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }

  if(PropertyResult == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "property not found");
    return NULL;
    }

  return Py_BuildValue(const_cast<char*>("s"), "");
}

static PyObject*
QtTesting_getQtVersion(PyObject* /*self*/, PyObject* /*args*/)
{
  // string QtTesting.getQtVersion()
  //    returns the Qt version as a string
  
  return Py_BuildValue(const_cast<char*>("s"), qVersion());
}


static PyObject*
QtTesting_getChildren(PyObject* /*self*/, PyObject* args)
{
  // string QtTesting.getChildren('object')
  //    returns the a list of strings with object names
  
  const char* object = 0;

  if(!PyArg_ParseTuple(args, const_cast<char*>("s"), &object))
    {
    return NULL;
    }

  PropertyObject = object;
  ObjectList.clear();

  if(Instance && QThread::currentThread() != QApplication::instance()->thread())
    {
    QMetaObject::invokeMethod(Instance, "threadGetChildren", Qt::QueuedConnection);
    if(!Instance->waitForGUI())
      {
      PyErr_SetString(PyExc_ValueError, "error getting children");
      return NULL;
      }
    }
  else if(QThread::currentThread() == QApplication::instance()->thread())
    {
    ObjectList = pqPythonEventSource::getChildren(PropertyObject);
    }
  else
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  if(PropertyObject == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }

  QString objs = ObjectList.join(", ");
  QString ret = QString("[%1]").arg(objs);

  return Py_BuildValue(const_cast<char*>("s"), 
             ret.toAscii().data());
}

static PyObject*
QtTesting_invokeMethod(PyObject* /*self*/, PyObject* args)
{
  // string QtTesting.invokeMethod('object', 'method')
  //    calls a method and returns its value
  
  const char* object = 0;
  const char* method = 0;

  if(!PyArg_ParseTuple(args, const_cast<char*>("ss"), &object, &method))
    {
    return NULL;
    }

  PropertyObject = object;
  PropertyValue = method;
  PropertyResult = QString();

  if(Instance && QThread::currentThread() != QApplication::instance()->thread())
    {
    QMetaObject::invokeMethod(Instance, "threadInvokeMethod", Qt::QueuedConnection);
    if(!Instance->waitForGUI())
      {
      PyErr_SetString(PyExc_ValueError, "error invoking method");
      return NULL;
      }
    }
  else if(QThread::currentThread() == QApplication::instance()->thread())
    {
    PropertyResult = pqPythonEventSource::invokeMethod(PropertyObject,
                                                       PropertyValue);
    }
  else
    {
    PyErr_SetString(PyExc_AssertionError, "pqPythonEventSource not defined");
    return NULL;
    }

  if(PropertyObject == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "object not found");
    return NULL;
    }
  else if(PropertyValue == QString::null)
    {
    PyErr_SetString(PyExc_ValueError, "method not found");
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
  {
    const_cast<char*>("setProperty"),
    QtTesting_setProperty,
    METH_VARARGS,
    const_cast<char*>("Set a property of an object.")
  },

  {
    const_cast<char*>("getQtVersion"),
    QtTesting_getQtVersion,
    METH_VARARGS,
    const_cast<char*>("Get the version of Qt being used.")
  },
  {
    const_cast<char*>("getChildren"),
    QtTesting_getChildren,
    METH_VARARGS,
    const_cast<char*>("Return a list of child objects.")
  },
  {
    const_cast<char*>("invokeMethod"),
    QtTesting_invokeMethod,
    METH_VARARGS,
    const_cast<char*>("Invoke a Qt slot with the signature \"QVariant foo()\".")
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
};

pqPythonEventSource::pqPythonEventSource(QObject* p)
  : pqThreadedEventSource(p)
{
  this->Internal = new pqInternal;
  int initPy = Py_IsInitialized();
  if(!initPy)
    {
    // initialize python
    Py_Initialize();
#ifdef SIGINT
    signal(SIGINT, SIG_DFL);
#endif
    }
  // add QtTesting to python's inittab, so it is
  // available to all interpreters
  PyImport_AppendInittab(const_cast<char*>("QtTesting"), initQtTesting);

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
  
QString pqPythonEventSource::getProperty(QString& object, QString& prop)
{
  // ensure other tasks have been completed
  pqEventDispatcher::processEventsAndWait(1);
  QVariant ret;

  QObject* qobject = pqObjectNaming::GetObject(object);
  if(!qobject)
    {
    object = QString::null;
    return QString();
    }
  int idx = qobject->metaObject()->indexOfProperty(prop.toAscii().data());
  if(idx == -1)
    {
    prop = QString::null;
    return QString();
    }
  else
    {
    QMetaProperty metaProp =  qobject->metaObject()->property(idx);
    ret = metaProp.read(qobject);

    if(metaProp.type() == QVariant::List || metaProp.type() ==
      QVariant::StringList)
      {
      return ret.toStringList().join(";");
      }
    return ret.toString();
    }
}


void pqPythonEventSource::threadGetProperty()
{
  PropertyValue = this->getProperty(PropertyObject, PropertyResult);
  this->guiAcknowledge();
}


void pqPythonEventSource::setProperty(QString& object, QString& prop, 
                                      const QString& value)
{
  // ensure other tasks have been completed
  pqEventDispatcher::processEventsAndWait(1);
  QVariant ret;

  QObject* qobject = pqObjectNaming::GetObject(object);
  if(!qobject)
    {
    object = QString::null;
    return;
    }
  
  int idx = qobject->metaObject()->indexOfProperty(prop.toAscii().data());
  if(idx == -1)
    {
    prop = QString::null;
    return;
    }
  else
    {
    QVariant val = value;
    QMetaProperty metaProp =  qobject->metaObject()->property(idx);
    if(metaProp.type() == QVariant::List || metaProp.type() ==
            QVariant::StringList)
      {
      val = value.split(";");
      }
    qobject->setProperty(prop.toAscii().data(), val);
    }
}

void pqPythonEventSource::threadSetProperty()
{
  this->setProperty(PropertyObject, PropertyResult, PropertyValue);
  this->guiAcknowledge();
}


QStringList pqPythonEventSource::getChildren(QString& object)
{
  // ensure other tasks have been completed
  pqEventDispatcher::processEventsAndWait(1);
  QStringList ret;

  QObject* qobject = pqObjectNaming::GetObject(object);
  if(!qobject)
    {
    object = QString::null;
    }
  else
    {
    const QObjectList& children = qobject->children();
    foreach(QObject* child, children)
      {
      ret.append(pqObjectNaming::GetName(*child));
      }
    }
  return ret;
}


void pqPythonEventSource::threadGetChildren()
{
  ObjectList = this->getChildren(PropertyObject);
  this->guiAcknowledge();
}

void pqPythonEventSource::start()
{
  PyEval_InitThreads();
  PyEval_ReleaseLock();
  pqThreadedEventSource::start();
}

void pqPythonEventSource::run()
{
  QFile file(this->Internal->FileName);
  if(!file.open(QFile::ReadOnly | QFile::Text))
    {
    printf("Unable to open python script\n");
    return;
    } 

  Instance = this;

  PyGILState_STATE gstate = PyGILState_Ensure();

  // finally run the script
  QByteArray wholeFile = file.readAll();
  int result = PyRun_SimpleString(wholeFile.data()) == 0 ? 0 : 1;

  PyGILState_Release(gstate);
  PyEval_ReleaseLock();

  this->done(result);
}

void pqPythonEventSource::threadInvokeMethod()
{
  PropertyResult = this->invokeMethod(PropertyObject, PropertyValue);
  this->guiAcknowledge();
}

QString pqPythonEventSource::invokeMethod(QString& object, QString& method)
{
  // ensure other tasks have been completed
  pqEventDispatcher::processEventsAndWait(1);
  QVariant ret;

  QObject* qobject = pqObjectNaming::GetObject(object);
  if(!qobject)
    {
    object = QString::null;
    }
  else
    {
    if(!QMetaObject::invokeMethod(qobject, method.toAscii().data(),
                                  Q_RETURN_ARG(QVariant, ret)))
      {
      method = QString::null;
      }
    }
  return ret.toString();
}

