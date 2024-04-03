// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInSituPythonConduitHelper.h"
#include "vtkInSituInitializationHelper.h"
#include "vtkLogger.h"

#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit && VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "catalyst_python_tools.h"
#endif

PyObject* vtkInSituPythonConduitHelper::GetCatalystParameters()
{
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit && VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#if CATALYST_WRAP_PYTHON
  conduit_node* node = vtkInSituInitializationHelper::GetCatalystParameters();
  if (!node)
  {
    vtkLogF(ERROR, "Catalyst node is not initialized");
    Py_RETURN_NONE;
  }

  PyObject* pynode = PyCatalystConduit_Node_Wrap(node, 0 /* do not pass ownership to python */);

  return pynode;
#else
  vtkLogF(ERROR, "ParaView was compiled against Catalyst without Python support");
  return nullptr;
#endif
#else
  vtkLogF(ERROR, "ParaView was compiled without Conduit and Python support");
  return nullptr;
#endif
}
