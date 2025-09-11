// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPythonCompleter.h"

#include "vtkSmartPyObject.h"

#include <QDebug>

#include <iostream>

QString pqPythonCompleter::getVariableToComplete(const QString& prompt)
{
  // Search backward through the string for usable characters
  QString textToComplete{};
  for (int i = prompt.length() - 1; i >= 0; --i)
  {
    QChar c = prompt.at(i);
    if (c.isLetterOrNumber() || c == '.' || c == '_' || c == '[' || c == ']' || c == '(' ||
      c == ')' || c == ' ' || c == ',' || c == '=' || c == '\'' || c == '"')
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

  // Variable to lookup is the name before the last dot or paren if there is one.
  QString lookup{};
  int dot = textToComplete.lastIndexOf('.');
  int paren = textToComplete.lastIndexOf('(');
  // look the first equal sign to extract the name in case a contructor is used
  // i.e. with prompt " s = Sphere( Radius=1,End<TAB> " we want to get "Sphere"
  int equal = textToComplete.indexOf('=') + 1;
  int maxPos = std::max<int>(dot, paren);
  if (maxPos != -1)
  {
    lookup = textToComplete.mid(equal, maxPos - equal);
    lookup = lookup.trimmed();
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
  int paren = compareText.lastIndexOf('(');
  int comma = compareText.lastIndexOf(',');
  int space = compareText.lastIndexOf(' ');
  int maxPos = std::max<int>({ dot, paren, comma, space });
  if (maxPos != -1)
  {
    compareText = compareText.mid(maxPos + 1);
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
void pqPythonCompleter::appendFunctionKeywordArguments(PyObject* function, QStringList& results)
{
  // Check if we have a function from paraview.simple
  vtkSmartPyObject simpleModule;
  simpleModule.TakeReference(PyImport_ImportModule("paraview.simple.session"));
  if (!simpleModule)
  {
    qWarning() << "Failed to import 'paraview.simple.session'";
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return;
    }
  }
  std::string argumentExtractorUtility;
  int hasPvTag = PyObject_HasAttrString(function, "__paraview_create_object_tag");
  if (hasPvTag)
  {
    // Verify the attribute is a bool with value True
    PyObject* pvtag = PyObject_GetAttrString(function, "__paraview_create_object_tag");
    if (pvtag && PyObject_IsTrue(pvtag))
    {
      // Hard-code this non-property default argument
      results.append("registrationName");
      argumentExtractorUtility = "ListProperties";
    }
  }
  else
  {
    argumentExtractorUtility = "_get_function_arguments";
  }

  vtkSmartPyObject getArguments(PyUnicode_FromString(argumentExtractorUtility.c_str()));
  vtkSmartPyObject retList(
    PyObject_CallMethodObjArgs(simpleModule.GetPointer(), getArguments, function, nullptr));
  if (!retList)
  {
    qWarning() << "Could not invoke 'paraview.simple.'" << argumentExtractorUtility.c_str();
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return;
    }
  }

  if (PyList_Check(retList))
  {
    const auto len = PyList_Size(retList);
    for (Py_ssize_t cc = 0; cc < len; ++cc)
    {
      PyObject* attributeItem = PyList_GetItem(retList, cc);
      Py_ssize_t size;
      const char* attributeName = PyUnicode_AsUTF8AndSize(attributeItem, &size);
      if (attributeName)
      {
        results.append(attributeName);
      }
    }
  }
}

//-----------------------------------------------------------------------------
PyObject* pqPythonCompleter::derivePyObject(const QString& pythonObjectName, PyObject* locals)
{
  PyObject* derivedObject = locals;
  QStringList tmpNames = pythonObjectName.split('.');
  int lastElement = tmpNames.size();
  if (lastElement > 0)
  {
    lastElement--;
  }

  for (int i = 0; i < tmpNames.size() && derivedObject; ++i)
  {
    QByteArray tmpName = tmpNames.at(i).toUtf8();
    PyObject* prevObj = derivedObject;
    // PyObject_Print(derivedObject, stdout, Py_PRINT_RAW);
    if (derivedObject && PyDict_Check(derivedObject))
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
