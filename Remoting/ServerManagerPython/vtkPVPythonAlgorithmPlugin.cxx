// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPython.h" // must be 1st include.

#include "vtkPVPythonAlgorithmPlugin.h"

#include "vtkObjectFactory.h"
#include "vtkPVPluginLoader.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMMessage.h"
#include "vtkSmartPyObject.h"
#include "vtksys/SystemTools.hxx"

#include <memory>
#include <stdexcept>

//============================================================================
// variables used by the Initializer should be only primitive types
// cannot be objects or their constructor will interfere with the Initializer
static int nifty_counter = 0;
vtkPVPythonAlgorithmPluginLoaderInitializer::vtkPVPythonAlgorithmPluginLoaderInitializer()
{
  if (nifty_counter == 0)
  {
    vtkPVPluginLoader::RegisterLoadPluginCallback(vtkPVPythonAlgorithmPlugin::LoadPlugin);
  }
  nifty_counter++;
}

vtkPVPythonAlgorithmPluginLoaderInitializer::~vtkPVPythonAlgorithmPluginLoaderInitializer() =
  default;

//============================================================================

class vtkPVPythonAlgorithmPlugin::vtkInternals
{
public:
  std::string PluginName;
  std::string PluginVersion;
  std::vector<std::string> XMLs;
};

//----------------------------------------------------------------------------
vtkPVPythonAlgorithmPlugin::vtkPVPythonAlgorithmPlugin(const char* fileName)
  : Internals(new vtkPVPythonAlgorithmPlugin::vtkInternals())
{
  this->Initialize(fileName, "load_plugin", nullptr);
}

vtkPVPythonAlgorithmPlugin::vtkPVPythonAlgorithmPlugin(
  const char* moduleName, const char* pythonCode)
  : Internals(new vtkPVPythonAlgorithmPlugin::vtkInternals())
{
  this->Initialize(moduleName, "load_plugin_from_string", pythonCode);
}

void vtkPVPythonAlgorithmPlugin::Initialize(
  const char* moduleOrFileName, const char* loadPlugin, const char* pythonCode)
{
  assert(moduleOrFileName != nullptr);

  // Initialize Python environment, if not already.
  vtkPythonInterpreter::Initialize();

  vtkInternals& internals = (*this->Internals);

  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject pvdetail(PyImport_ImportModule("paraview.detail.pythonalgorithm"));
  if (!pvdetail)
  {
    throw std::runtime_error("Failed to import `paraview.detail.pythonalgorithm`.");
  }

  vtkSmartPyObject load_plugin(PyObject_GetAttrString(pvdetail, loadPlugin));
  if (!load_plugin)
  {
    throw std::runtime_error(
      std::string("Failed to locate `paraview.detail.pythonalgorithm.") + loadPlugin + "`.");
  }

  PyObject* obj = nullptr;
  if (pythonCode)
  {
    vtkSmartPyObject moduleName(PyUnicode_FromString(moduleOrFileName));
    vtkSmartPyObject python_code(PyUnicode_FromString(pythonCode));
    obj = PyObject_CallFunctionObjArgs(
      load_plugin, moduleName.GetPointer(), python_code.GetPointer(), nullptr);
  }
  else
  {
    vtkSmartPyObject fileName(PyUnicode_FromString(moduleOrFileName));
    obj = PyObject_CallFunctionObjArgs(load_plugin, fileName.GetPointer(), nullptr);
  }
  vtkSmartPyObject module(obj);
  if (!module)
  {
    throw std::runtime_error(
      std::string("Failed to call `paraview.detail.pythonalgorithm.") + loadPlugin + "`");
  }

  vtkSmartPyObject get_plugin_xmls(PyObject_GetAttrString(pvdetail, "get_plugin_xmls"));
  if (!get_plugin_xmls)
  {
    throw std::runtime_error("Failed to locate `paraview.detail.pythonalgorithm.get_plugin_xmls`.");
  }

  vtkSmartPyObject result(
    PyObject_CallFunctionObjArgs(get_plugin_xmls, module.GetPointer(), nullptr));
  if (!result)
  {
    throw std::runtime_error("Failed to call `paraview.detail.pythonalgorithm.get_plugin_xmls`.");
  }

  if (PyList_Check(result))
  {
    auto numitems = PyList_GET_SIZE(result.GetPointer());
    for (decltype(numitems) cc = 0; cc < numitems; ++cc)
    {
      PyObject* borrowed_item = PyList_GET_ITEM(result.GetPointer(), cc);
      if (PyUnicode_Check(borrowed_item))
      {
        internals.XMLs.push_back(PyUnicode_AsUTF8(borrowed_item));
      }
    }
  }

  vtkSmartPyObject get_plugin_name(PyObject_GetAttrString(pvdetail, "get_plugin_name"));
  if (!get_plugin_name)
  {
    throw std::runtime_error("Failed to locate `paraview.detail.pythonalgorithm.get_plugin_name`.");
  }

  result.TakeReference(PyObject_CallFunctionObjArgs(get_plugin_name, module.GetPointer(), nullptr));
  if (!result || !PyUnicode_Check(result))
  {
    throw std::runtime_error("Failed to call `paraview.detail.pythonalgorithm.get_plugin_name`.");
  }

  internals.PluginName = PyUnicode_AsUTF8(result);

  vtkSmartPyObject get_plugin_version(PyObject_GetAttrString(pvdetail, "get_plugin_version"));
  if (!get_plugin_version)
  {
    throw std::runtime_error(
      "Failed to locate `paraview.detail.pythonalgorithm.get_plugin_version`.");
  }

  result.TakeReference(
    PyObject_CallFunctionObjArgs(get_plugin_version, module.GetPointer(), nullptr));
  if (!result || !PyUnicode_Check(result))
  {
    throw std::runtime_error(
      "Failed to call `paraview.detail.pythonalgorithm.get_plugin_version`.");
  }

  internals.PluginVersion = PyUnicode_AsUTF8(result);

  this->SetFileName(moduleOrFileName);
}

//----------------------------------------------------------------------------
vtkPVPythonAlgorithmPlugin::~vtkPVPythonAlgorithmPlugin() = default;

//----------------------------------------------------------------------------
const char* vtkPVPythonAlgorithmPlugin::GetPluginName()
{
  const vtkInternals& internals = (*this->Internals);
  return internals.PluginName.empty() ? nullptr : internals.PluginName.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPVPythonAlgorithmPlugin::GetPluginVersionString()
{
  const vtkInternals& internals = (*this->Internals);
  return internals.PluginVersion.empty() ? nullptr : internals.PluginVersion.c_str();
}

//----------------------------------------------------------------------------
void vtkPVPythonAlgorithmPlugin::GetXMLs(std::vector<std::string>& xmls)
{
  const vtkInternals& internals = (*this->Internals);
  xmls = internals.XMLs;
}

//----------------------------------------------------------------------------
bool vtkPVPythonAlgorithmPlugin::LoadPlugin(const char* pname)
{
  if (vtksys::SystemTools::GetFilenameLastExtension(pname) == ".py")
  {
    try
    {
      auto plugin = new vtkPVPythonAlgorithmPlugin(pname);
      return vtkPVPlugin::ImportPlugin(plugin);
    }
    catch (const std::runtime_error& err)
    {
      vtkGenericWarningMacro("Failed to load Python plugin:\n" << err.what());
      vtkPythonScopeGilEnsurer gilEnsurer;
      if (PyErr_Occurred() != nullptr)
      {
        PyErr_Print();
        PyErr_Clear();
      }
      return false;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVPythonAlgorithmPlugin::InitializeFromStringAndGetXMLs(
  const char* moduleName, const char* pythonCode, std::vector<std::string>& xmls)
{
  try
  {
    auto plugin = std::make_unique<vtkPVPythonAlgorithmPlugin>(moduleName, pythonCode);
    plugin->GetXMLs(xmls);
    return true;
  }
  catch (const std::runtime_error& err)
  {
    vtkGenericWarningMacro("Failed to load Python plugin:\n" << err.what());
    vtkPythonScopeGilEnsurer gilEnsurer;
    if (PyErr_Occurred() != nullptr)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return false;
  }
  return false;
}
