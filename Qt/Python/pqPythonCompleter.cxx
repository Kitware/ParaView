// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPythonCompleter.h"

QString pqPythonCompleter::getVariableToComplete(const QString& prompt)
{
  // Search backward through the string for usable characters
  QString textToComplete{};
  for (int i = prompt.length() - 1; i >= 0; --i)
  {
    QChar c = prompt.at(i);
    if (c.isLetterOrNumber() || c == '.' || c == '_' || c == '[' || c == ']')
    {
      textToComplete.prepend(c);
    }
    else
    {
      break;
    }
  }
  return textToComplete;
}

//-----------------------------------------------------------------------------
QStringList pqPythonCompleter::getCompletions(const QString& prompt)
{
  QString textToComplete = this->getVariableToComplete(prompt);

  // Variable to lookup is the name before the last dot if there is one.
  QString lookup{};
  int dot = textToComplete.lastIndexOf('.');
  if (dot != -1)
  {
    lookup = textToComplete.mid(0, dot);
  }

  // Lookup python names
  if (this->getCompleteEmptyPrompts() || !lookup.isEmpty() ||
    !this->getCompletionPrefix(prompt).isEmpty())
  {
    return this->getPythonCompletions(lookup);
  }

  return QStringList{};
};

//-----------------------------------------------------------------------------
QString pqPythonCompleter::getCompletionPrefix(const QString& prompt)
{
  // Search backward through the string for usable characters
  QString compareText = this->getVariableToComplete(prompt);
  int dot = compareText.lastIndexOf('.');
  if (dot != -1)
  {
    compareText = compareText.mid(dot + 1);
  }

  return compareText;
};

//-----------------------------------------------------------------------------
void pqPythonCompleter::appendPyObjectAttributes(PyObject* object, QStringList& results)
{
  if (!object)
  {
    return;
  }

  PyObject* keys = nullptr;
  bool is_dict = PyDict_Check(object);
  if (is_dict)
  {
    keys = PyDict_Keys(object); // returns *new* reference.
  }
  else
  {
    keys = PyObject_Dir(object); // returns *new* reference.
  }
  if (keys)
  {
    PyObject* key;
    PyObject* value;
    QString keystr;
    int nKeys = PyList_Size(keys);
    for (int i = 0; i < nKeys; ++i)
    {
      key = PyList_GetItem(keys, i);
      if (is_dict)
      {
        value = PyDict_GetItem(object, key); // Return value: Borrowed reference.
        Py_XINCREF(value);                   // so we can use Py_DECREF later.
      }
      else
      {
        value = PyObject_GetAttr(object, key); // Return value: New reference.
      }
      if (!value)
      {
        continue;
      }
      results << PyUnicode_AsUTF8(key);
      Py_DECREF(value);

      // Clear out any errors that may have occurred.
      PyErr_Clear();
    }
    Py_DECREF(keys);
  }
  Py_DECREF(object);
}

//-----------------------------------------------------------------------------
PyObject* pqPythonCompleter::derivePyObject(const QString& pythonObjectName, PyObject* locals)
{
  PyObject* derivedObject = locals;
  QStringList tmpNames = pythonObjectName.split('.');
  for (int i = 0; i < tmpNames.size() && derivedObject; ++i)
  {
    QByteArray tmpName = tmpNames.at(i).toUtf8();
    PyObject* prevObj = derivedObject;
    if (PyDict_Check(derivedObject))
    {
      derivedObject = PyDict_GetItemString(derivedObject, tmpName.data());
      Py_XINCREF(derivedObject);
    }
    else
    {
      derivedObject = PyObject_GetAttrString(derivedObject, tmpName.data());
    }
    Py_DECREF(prevObj);
  }
  PyErr_Clear();

  return derivedObject;
}
