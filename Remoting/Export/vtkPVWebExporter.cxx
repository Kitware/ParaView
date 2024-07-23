// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVWebExporter.h"

#include "vtkObjectFactory.h"

#include "vtkPVFileInformation.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

namespace
{
const std::string GLANCE_HTML_LOCATION = "/web/glance/ParaViewGlance.html";
}

vtkStandardNewMacro(vtkPVWebExporter);
//----------------------------------------------------------------------------
vtkPVWebExporter::vtkPVWebExporter() = default;

//----------------------------------------------------------------------------
vtkPVWebExporter::~vtkPVWebExporter() = default;

//----------------------------------------------------------------------------
void vtkPVWebExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExportToGlance ? " << (this->ExportToGlance ? "yes" : "no") << endl;
  os << indent << "AutomaticGlanceHTML ? " << (this->AutomaticGlanceHTML ? "yes" : "no") << endl;
  os << indent << "ParaViewGlanceHTML: " << this->ParaViewGlanceHTML << endl;
  os << indent << "DisableNetwork ?" << (this->DisableNetwork ? "yes" : "no") << endl;
}

//----------------------------------------------------------------------------
void vtkPVWebExporter::Write()
{
  this->Superclass::Write();

  std::string glanceHTML;
  if (this->ExportToGlance)
  {
    if (this->AutomaticGlanceHTML)
    {
      glanceHTML =
        vtkPVFileInformation::GetParaViewSharedResourcesDirectory() + ::GLANCE_HTML_LOCATION;
    }
    else
    {
      glanceHTML = this->ParaViewGlanceHTML;
    }
    if (!vtksys::SystemTools::FileExists(glanceHTML.c_str()))
    {
      vtkWarningMacro(
        "Could not find file " << glanceHTML << ", not writing ParaView Glance HTML file");
      glanceHTML = "";
    }
  }

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

    PyObject_CallMethod(
      pvmodule, const_cast<char*>("applyParaViewNaming"), const_cast<char*>("(s)"), this->FileName);
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
      const_cast<char*>("(s)"), this->FileName);
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs file");
      throw 1;
    }

    if (!glanceHTML.empty())
    {
      PyObject_CallMethod(module, const_cast<char*>("addDataToViewer"), const_cast<char*>("(ssb)"),
        this->FileName, const_cast<char*>(glanceHTML.c_str()), this->DisableNetwork);
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
