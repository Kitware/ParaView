// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVWebExporter.h"

#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

vtkStandardNewMacro(vtkPVWebExporter);
//----------------------------------------------------------------------------
vtkPVWebExporter::vtkPVWebExporter()
{
  this->ParaViewGlanceHTML = nullptr;
}

//----------------------------------------------------------------------------
vtkPVWebExporter::~vtkPVWebExporter()
{
  delete[] this->ParaViewGlanceHTML;
}

//----------------------------------------------------------------------------
void vtkPVWebExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVWebExporter::Write()
{
  this->Superclass::Write();

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore

  // ensure Python interpreter is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  try
  {
    vtkSmartPyObject pvmodule(PyImport_ImportModule("paraview.web.vtkjs_helper"));
    if (!pvmodule || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import paraview.web.vtkjs_helper module.");
      throw 1;
    }

    PyObject_CallMethod(pvmodule, const_cast<char*>("applyParaViewNaming"),
      const_cast<char*>("(s)"), const_cast<char*>(this->FileName));
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to rename datasets using ParaView proxy name");
      throw 1;
    }

#if VTK_MODULE_ENABLE_VTK_WebPython
    vtkSmartPyObject module(PyImport_ImportModule("vtkmodules.web.vtkjs_helper"));
    if (!module || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import vtkmodules.web.vtkjs_helper module.");
      throw 1;
    }

    PyObject_CallMethod(module, const_cast<char*>("convertDirectoryToZipFile"),
      const_cast<char*>("(s)"), const_cast<char*>(this->FileName));
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs file");
      throw 1;
    }

    if (this->ParaViewGlanceHTML)
    {
      PyObject_CallMethod(module, const_cast<char*>("addDataToViewer"), const_cast<char*>("(ss)"),
        const_cast<char*>(this->FileName), const_cast<char*>(this->ParaViewGlanceHTML));
    }
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs data file into HTML viewer");
      throw 1;
    }
#else
    vtkGenericWarningMacro("The VTK::WebPython module was not enabled.");
    throw 1;
#endif
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
  }
#endif
}
