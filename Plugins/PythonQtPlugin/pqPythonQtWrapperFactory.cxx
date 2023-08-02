// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPythonQtWrapperFactory.h"

#include <vtkObject.h>
#include <vtkPythonUtil.h>

//-----------------------------------------------------------------------------
PyObject* pqPythonQtWrapperFactory::wrap(const QByteArray& classname, void* ptr)
{
  if (classname.startsWith("vtk"))
  {
    return vtkPythonUtil::GetObjectFromPointer(static_cast<vtkObjectBase*>(ptr));
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void* pqPythonQtWrapperFactory::unwrap(const QByteArray& classname, PyObject* object)
{
  if (classname.startsWith("vtk"))
  {
    return vtkPythonUtil::GetPointerFromObject(object, classname.data());
  }
  return nullptr;
}
