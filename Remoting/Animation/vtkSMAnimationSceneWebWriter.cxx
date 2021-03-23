/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneWebWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneWebWriter.h"

#include "vtkJSONSceneExporter.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#include <assert.h>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
struct vtkSMAnimationSceneWebWriter::vtkInternals
{
  std::stringstream RootIndexStr;
  std::stringstream ObjIndexStr;
  std::size_t FrameCounter;
  vtkNew<vtkJSONSceneExporter> Exporter;

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkSmartPyObject PyModulePV;
#endif
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMAnimationSceneWebWriter);

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWebWriter::vtkSMAnimationSceneWebWriter()
  : Internals(new vtkInternals)
{
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWebWriter::~vtkSMAnimationSceneWebWriter()
{
  delete Internals;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWebWriter::SetRenderView(vtkSMRenderViewProxy* rv)
{
  this->RenderView = rv;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneWebWriter::SaveInitialize(int vtkNotUsed(startCount))
{
  if (this->RenderView == nullptr)
  {
    vtkErrorMacro("No view from which to save the geometry is set.");
    return false;
  }

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  try
  {
    // ensure Python interpreter is initialized.
    vtkPythonInterpreter::Initialize();
    vtkPythonScopeGilEnsurer gilEnsurer;
    this->Internals->PyModulePV =
      vtkSmartPyObject(PyImport_ImportModule("paraview.web.vtkjs_helper"));
    if (!this->Internals->PyModulePV || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import paraview.web.vtkjs_helper module.");
      throw 1;
    }
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }

  this->Internals->FrameCounter = 0;
  this->Internals->RootIndexStr << "{\n"
                                << "  \"version\": 1.0,\n"
                                << "  \"background\": [ ],\n"
                                << "  \"camera\": { },\n"
                                << "  \"centerOfRotation\": { },\n"
                                << "  \"scene\": [ ],\n"
                                << "  \"lookupTables\": { },\n"
                                << "  \"animation\": {\n"
                                << "    \"type\": \"vtkTimeStepBasedAnimationHandler\",\n"
                                << "    \"timeSteps\": [\n";

  return true;
#else
  vtkErrorMacro("Python is not enabled, cannot export scene");
  return false;
#endif
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneWebWriter::SaveFrame(double time)
{
  std::size_t counter = this->Internals->FrameCounter;
  vtkJSONSceneExporter* exporter = this->Internals->Exporter;

  // Export a folder containing data for the current timestep
  const std::string filename = this->GetFileName() + std::string(".") + std::to_string(counter);
  exporter->SetRenderWindow(this->RenderView->GetRenderWindow());
  exporter->SetFileName(filename.c_str());
  exporter->Write();

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  try
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    PyObject_CallMethod(this->Internals->PyModulePV, const_cast<char*>("applyParaViewNaming"),
      const_cast<char*>("(s)"), filename.c_str());
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to apply paraview naming on current timestep");
      throw 1;
    }
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }
#else
  vtkErrorMacro("Python is not enabled, cannot export scene");
  return false;
#endif

  // Write the root json information
  std::stringstream& fileStr = this->Internals->RootIndexStr;
  // if not the first step add missing comma from the last one
  if (counter != 0)
  {
    fileStr << ",\n";
  }
  fileStr << "      {\n"
          << "        \"time\": " << std::showpoint << time << "\n"
          << "      }";
  this->Internals->FrameCounter++;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneWebWriter::SaveFinalize()
{
  // Write the root json information
  std::stringstream& fileStr = this->Internals->RootIndexStr;
  fileStr << "\n    ]\n"
          << "  }\n"
          << "}\n";
  const std::string filePath =
    vtksys::SystemTools::GetParentDirectory(this->FileName) + "/index.json";
  vtksys::ofstream file;
  file.open(filePath.c_str(), std::ios::out);
  file << fileStr.str().c_str();
  file.close();

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkPythonScopeGilEnsurer gilEnsurer;
  try
  {
    vtkSmartPyObject module(PyImport_ImportModule("vtk.web.vtkjs_helper"));
    if (!module || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import vtk.web.vtkjs_helper module.");
      throw 1;
    }

    PyObject_CallMethod(module, const_cast<char*>("zipAllTimeSteps"), const_cast<char*>("(s)"),
      const_cast<char*>(this->FileName));
    if (PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to bundle vtkjs file");
      throw 1;
    }
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }
#else
  vtkErrorMacro("Python is not enabled, cannot export scene");
  return false;
#endif

  return true;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWebWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
}
