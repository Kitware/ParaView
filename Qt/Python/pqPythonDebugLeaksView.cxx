// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "pqPythonDebugLeaksView.h"
#include "pqPythonShell.h"
#include "vtkPythonUtil.h"
#include "vtkQtDebugLeaksModel.h"
#include "vtkSmartPyObject.h"

//-----------------------------------------------------------------------------
namespace
{

PyObject* consoleContext(pqPythonShell* shell)
{
  return shell ? static_cast<PyObject*>(shell->consoleLocals()) : nullptr;
}
};

//-----------------------------------------------------------------------------
pqPythonDebugLeaksView::pqPythonDebugLeaksView(QWidget* p)
  : vtkQtDebugLeaksView(p)
{
}

//-----------------------------------------------------------------------------
pqPythonDebugLeaksView::~pqPythonDebugLeaksView() = default;

//-----------------------------------------------------------------------------
void pqPythonDebugLeaksView::setShell(pqPythonShell* shell)
{
  this->Shell = shell;
}

//-----------------------------------------------------------------------------
pqPythonShell* pqPythonDebugLeaksView::shell() const
{
  return this->Shell;
}

//-----------------------------------------------------------------------------
void pqPythonDebugLeaksView::onObjectDoubleClicked(vtkObjectBase* object)
{
  this->addObjectToPython(object);
}

//-----------------------------------------------------------------------------
void pqPythonDebugLeaksView::onClassNameDoubleClicked(const QString& className)
{
  this->addObjectsToPython(this->model()->getObjects(className));
}

//-----------------------------------------------------------------------------
void pqPythonDebugLeaksView::addObjectToPython(vtkObjectBase* object)
{
  if (auto context = consoleContext(this->shell()))
  {
    vtkSmartPyObject pyObj(vtkPythonUtil::GetObjectFromPointer(object));
    PyDict_SetItemString(context, "obj", pyObj);
  }
}

//-----------------------------------------------------------------------------
void pqPythonDebugLeaksView::addObjectsToPython(const QList<vtkObjectBase*>& objects)
{
  if (auto context = consoleContext(this->shell()))
  {
    vtkSmartPyObject pyListObj(PyList_New(objects.size()));
    for (int i = 0; i < objects.size(); ++i)
    {
      PyObject* pyObj = vtkPythonUtil::GetObjectFromPointer(objects[i]);
      PyList_SET_ITEM(pyListObj.GetPointer(), i, pyObj);
    }
    PyDict_SetItemString(context, "objs", pyListObj);
  }
}
