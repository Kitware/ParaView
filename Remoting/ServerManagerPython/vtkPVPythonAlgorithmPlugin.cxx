/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPythonAlgorithmPlugin.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be 1st include.

#include "vtkPVPythonAlgorithmPlugin.h"

#include "vtkObjectFactory.h"
#include "vtkPVPluginLoader.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMMessage.h"
#include "vtkSmartPyObject.h"
#include "vtksys/SystemTools.hxx"

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
vtkPVPythonAlgorithmPlugin::vtkPVPythonAlgorithmPlugin(const char* modulefile)
  : Internals(new vtkPVPythonAlgorithmPlugin::vtkInternals())
{
  assert(modulefile != nullptr);

  // Initialize Python environment, if not already.
  vtkPythonInterpreter::Initialize();

  vtkInternals& internals = (*this->Internals);

  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject pvdetail(PyImport_ImportModule("paraview.detail.pythonalgorithm"));
  if (!pvdetail)
  {
    throw std::runtime_error("Failed to import `paraview.detail.pythonalgorithm`.");
  }

  vtkSmartPyObject load_plugin(PyObject_GetAttrString(pvdetail, "load_plugin"));
  if (!load_plugin)
  {
    throw std::runtime_error("Failed to locate `paraview.detail.pythonalgorithm.load_plugin`.");
  }

  vtkSmartPyObject filename(PyString_FromString(modulefile));
  vtkSmartPyObject module(
    PyObject_CallFunctionObjArgs(load_plugin, filename.GetPointer(), nullptr));
  if (!module)
  {
    throw std::runtime_error("Failed to call `paraview.detail.pythonalgorithm.load_plugin`.");
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
      if (PyString_Check(borrowed_item))
      {
        internals.XMLs.push_back(PyString_AsString(borrowed_item));
      }
    }
  }

  vtkSmartPyObject get_plugin_name(PyObject_GetAttrString(pvdetail, "get_plugin_name"));
  if (!get_plugin_name)
  {
    throw std::runtime_error("Failed to locate `paraview.detail.pythonalgorithm.get_plugin_name`.");
  }

  result.TakeReference(PyObject_CallFunctionObjArgs(get_plugin_name, module.GetPointer(), nullptr));
  if (!result || !PyString_Check(result))
  {
    throw std::runtime_error("Failed to call `paraview.detail.pythonalgorithm.get_plugin_name`.");
  }

  internals.PluginName = PyString_AsString(result);

  vtkSmartPyObject get_plugin_version(PyObject_GetAttrString(pvdetail, "get_plugin_version"));
  if (!get_plugin_version)
  {
    throw std::runtime_error(
      "Failed to locate `paraview.detail.pythonalgorithm.get_plugin_version`.");
  }

  result.TakeReference(
    PyObject_CallFunctionObjArgs(get_plugin_version, module.GetPointer(), nullptr));
  if (!result || !PyString_Check(result))
  {
    throw std::runtime_error(
      "Failed to call `paraview.detail.pythonalgorithm.get_plugin_version`.");
  }

  internals.PluginVersion = PyString_AsString(result);

  this->SetFileName(modulefile);
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
      auto* plugin = new vtkPVPythonAlgorithmPlugin(pname);
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
