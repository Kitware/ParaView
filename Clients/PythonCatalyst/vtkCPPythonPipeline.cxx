/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPython.h" // must be first

#include "vtkCPPythonPipeline.h"

#include "vtkCPPythonScriptPipeline.h"
#include "vtkCPPythonScriptV2Pipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkPVLogger.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

extern "C" {
void vtkPVInitializePythonModules();
}

namespace
{
size_t non_empty_strlen(std::string data)
{
  data.erase(
    std::remove_if(data.begin(), data.end(), [](unsigned char x) { return std::isspace(x); }),
    data.end());
  return data.size();
}
}

//----------------------------------------------------------------------------
namespace
{
void CatalystInitializePython()
{
  static bool initialized = false;
  if (initialized)
  {
    return;
  }
  initialized = true;

  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  // initialize Python environment.
  vtkSmartPyObject module(PyImport_ImportModule("paraview.catalyst.detail"));
  if (!module)
  {
    PyErr_Print();
    PyErr_Clear();
  }

  vtkSmartPyObject method(PyString_FromString("InitializePythonEnvironment"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(module, method, nullptr));
  if (!result)
  {
    PyErr_Print();
    PyErr_Clear();
  }
}
}
//----------------------------------------------------------------------------
vtkCPPythonPipeline::vtkCPPythonPipeline()
{
  CatalystInitializePython();
}

//----------------------------------------------------------------------------
vtkCPPythonPipeline::~vtkCPPythonPipeline()
{
}

//----------------------------------------------------------------------------
int vtkCPPythonPipeline::DetectScriptVersion(const char* fname)
{
  auto contr = vtkMultiProcessController::GetGlobalController();
  if (contr && contr->GetLocalProcessId() > 0)
  {
    int version = 0;
    contr->Broadcast(&version, 1, 0);
    return version;
  }

  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "DetectScriptVersion '%s'", fname);
  std::ifstream ifp(fname);

  int version = 0;
  if (vtksys::SystemTools::FileIsDirectory(fname) ||
    vtksys::SystemTools::GetFilenameLastExtension(fname) == ".zip")
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "File is a directory or a zip archive; must be version 2.0");
    version = 2;
  }
  else if (!vtksys::SystemTools::FileExists(fname, /*isFile*/ true))
  {
    vtkVLogF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "File does not exist; could not determine version.");
    version = 0;
  }
  else if (ifp.is_open())
  {
    vtksys::RegularExpression regex1("paraview version ([0-9]+)\\.([0-9]+)");
    vtksys::RegularExpression regex2("#[ ]+script-version: ([0-9]+)");
    std::array<char, 1024> buffer;
    while (ifp.getline(&buffer[0], 1024))
    {
      if (regex1.find(&buffer[0]))
      {
        int major = std::atoi(regex1.match(1).c_str());
        int minor = std::atoi(regex1.match(2).c_str());
        if (major > 5 || (major == 5 && minor >= 9))
        {
          vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
            "File written using ParaView (%d, %d). Treating as version=2", major, minor);
          version = 2;
        }
        else
        {
          vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
            "File written using ParaView (%d, %d). Treating as version=1", major, minor);
          version = 1;
        }
        break;
      }
      else if (regex2.find(&buffer[0]))
      {
        vtkVLogF(
          PARAVIEW_LOG_CATALYST_VERBOSITY(), "Found 'script-version: %s'", regex2.match(1).c_str());
        if (std::atoi(regex2.match(1).c_str()) == 2)
        {
          version = 2;
        }
        else if (std::atoi(regex2.match(1).c_str()) == 1)
        {
          version = 1;
        }
        else
        {
          vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "invalid 'script-version' specified.");
        }
        break;
      }
      else if (buffer[0] == '#' || non_empty_strlen(&buffer[0]) == 0)
      {
        // empty or comment line, continue.
        continue;
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "regex match failed for '%s'", &buffer[0]);
        break;
      }
    }
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Cannot read file; could not determine version.");
  }

  if (contr && contr->GetNumberOfProcesses() > 1)
  {
    assert(contr->GetLocalProcessId() == 0);
    contr->Broadcast(&version, 1, 0);
  }

  return version;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCPPythonPipeline> vtkCPPythonPipeline::CreatePipeline(
  const char* fname, int default_version)
{
  int version = vtkCPPythonPipeline::DetectScriptVersion(fname);
  if (version == 0)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "Failed to determine version. Using default version %d", default_version);
    version = default_version;
  }

  if (version == 1)
  {
    return vtkSmartPointer<vtkCPPythonScriptPipeline>::New();
  }
  else if (version == 2)
  {
    return vtkSmartPointer<vtkCPPythonScriptV2Pipeline>::New();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCPPythonPipeline> vtkCPPythonPipeline::CreateAndInitializePipeline(
  const char* fname, int default_version)
{
  if (auto pipeline = vtkCPPythonPipeline::CreatePipeline(fname, default_version))
  {
    if (auto v1 = vtkCPPythonScriptPipeline::SafeDownCast(pipeline))
    {
      if (v1->Initialize(fname))
      {
        return v1;
      }
    }
    else if (auto v2 = vtkCPPythonScriptV2Pipeline::SafeDownCast(pipeline))
    {
      if (v2->Initialize(fname))
      {
        return v2;
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkCPPythonPipeline::FixEOL(std::string& str)
{
  const std::string from = "\\n";
  const std::string to = "\\\\n";
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  // sanitize triple quotes while we are at it
  const std::string from2 = "\"\"\"";
  const std::string to2 = "'''";
  start_pos = 0;
  while ((start_pos = str.find(from2, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from2.length(), to2);
    start_pos += to.length();
  }
  return;
}

//----------------------------------------------------------------------------
std::string vtkCPPythonPipeline::GetPythonAddress(void* pointer)
{
  char addressOfPointer[1024];
#ifdef COPROCESSOR_WIN32_BUILD
  sprintf_s(addressOfPointer, "%p", pointer);
#else
  sprintf(addressOfPointer, "%p", pointer);
#endif
  char* aplus = addressOfPointer;
  if ((addressOfPointer[0] == '0') && ((addressOfPointer[1] == 'x') || addressOfPointer[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }

  std::string value = aplus;
  return value;
}

//----------------------------------------------------------------------------
vtkCPPythonPipeline* vtkCPPythonPipeline::NewPipeline(const char* fname, int default_version)
{
  if (auto sptr = vtkCPPythonPipeline::CreatePipeline(fname, default_version))
  {
    sptr->Register(nullptr);
    return sptr;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkCPPythonPipeline* vtkCPPythonPipeline::NewAndInitializePipeline(
  const char* fname, int default_version)
{
  if (auto sptr = vtkCPPythonPipeline::CreateAndInitializePipeline(fname, default_version))
  {
    sptr->Register(nullptr);
    return sptr;
  }
  return nullptr;
}
