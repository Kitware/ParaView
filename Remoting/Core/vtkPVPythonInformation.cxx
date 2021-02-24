/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Include vtkPython.h first to avoid warnings:
#include "vtkPVConfig.h"
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h"

#include "vtkNew.h"
#include "vtkPythonInterpreter.h"
#include "vtkSmartPyObject.h"
#include "vtkStringOutputWindow.h"
#endif

#include "vtkClientServerStream.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVPythonInformation.h"

#include <algorithm>
#include <sstream>

vtkStandardNewMacro(vtkPVPythonInformation);

//----------------------------------------------------------------------------
vtkPVPythonInformation::vtkPVPythonInformation()
  : PythonSupport(false)
  , NumpySupport(false)
  , MatplotlibSupport(false)
{
}

//----------------------------------------------------------------------------
vtkPVPythonInformation::~vtkPVPythonInformation() = default;

//----------------------------------------------------------------------------
void vtkPVPythonInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

#define PRINT_IVAR(_ivarName) os << indent << #_ivarName ": " << this->_ivarName << endl

  PRINT_IVAR(PythonSupport);
  if (this->PythonSupport)
  {
    PRINT_IVAR(PythonPath);
    PRINT_IVAR(PythonVersion);

    PRINT_IVAR(NumpySupport);
    if (this->NumpySupport)
    {
      PRINT_IVAR(NumpyPath);
      PRINT_IVAR(NumpyVersion);
    }

    PRINT_IVAR(MatplotlibSupport);
    if (this->MatplotlibSupport)
    {
      PRINT_IVAR(MatplotlibPath);
      PRINT_IVAR(MatplotlibVersion);
    }
  }
#undef PRINT_IVAR
}

//----------------------------------------------------------------------------
void vtkPVPythonInformation::DeepCopy(vtkPVPythonInformation* info)
{
#define COPY_IVAR_SETGET(_ivarName) this->Set##_ivarName(info->Get##_ivarName())

  COPY_IVAR_SETGET(PythonSupport);
  COPY_IVAR_SETGET(PythonPath);
  COPY_IVAR_SETGET(PythonVersion);

  COPY_IVAR_SETGET(NumpySupport);
  COPY_IVAR_SETGET(NumpyPath);
  COPY_IVAR_SETGET(NumpyVersion);

  COPY_IVAR_SETGET(MatplotlibSupport);
  COPY_IVAR_SETGET(MatplotlibPath);
  COPY_IVAR_SETGET(MatplotlibVersion);

#undef COPY_IVAR_SETGET
}

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_WrappingPythonCore
namespace
{

void flushAndClearErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print(); // print implies PyErr_Clear().
  }
}

bool hasModule(const char* module)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mod(PyImport_ImportModule(module));
  flushAndClearErrors();
  return mod;
}

// Returns empty string on error.
std::string getModuleAttrAsString(const char* module, const char* attribute)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject mod(PyImport_ImportModule(module));
  flushAndClearErrors();
  if (!mod)
  {
    std::ostringstream result;
    result << "(module '" << module << "' not found)";
    return result.str();
  }

  vtkSmartPyObject attr(PyObject_GetAttrString(mod, attribute));
  flushAndClearErrors();
  if (!attr)
  {
    std::ostringstream result;
    result << "('" << module << "' module found, missing '" << attribute << "' attribute)";
    return result.str();
  }

  const char* cdata = PyUnicode_AsUTF8(attr);
  std::string result(cdata ? cdata : "");
  return result;
}

std::string chopFilename(const std::string& path)
{
  std::string::size_type pos = path.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    return std::string(path.begin(), path.begin() + pos);
  }
  return path;
}
}
#endif

//----------------------------------------------------------------------------
void vtkPVPythonInformation::CopyFromObject(vtkObject* vtkNotUsed(obj))
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  this->SetPythonSupport(true);

  vtkPythonInterpreter::Initialize();

  // Find a core library path and chop off the module specific bits.
  // sys.executable and such all return paraview/pvserver/etc/etc.
  this->SetPythonPath(chopFilename(getModuleAttrAsString("os", "__file__")));

  // Recover python version and remove the end of line within it
  std::string pythonVersion = getModuleAttrAsString("sys", "version");
  std::replace(pythonVersion.begin(), pythonVersion.end(), '\n', ' ');
  this->SetPythonVersion(pythonVersion);

  // while testing for modules, we don't want the error messages to be reported
  // on to the terminal, so capture them.
  vtkNew<vtkStringOutputWindow> captureErrors;
  vtkSmartPointer<vtkOutputWindow> oldWindow = vtkOutputWindow::GetInstance();
  vtkOutputWindow::SetInstance(captureErrors);

  this->SetNumpySupport(hasModule("numpy"));
  if (this->NumpySupport)
  {
    this->SetNumpyPath(chopFilename(getModuleAttrAsString("numpy", "__file__")));
    this->SetNumpyVersion(getModuleAttrAsString("numpy", "__version__"));
  }

  this->SetMatplotlibSupport(hasModule("matplotlib"));
  if (this->MatplotlibSupport)
  {
    this->SetMatplotlibPath(chopFilename(getModuleAttrAsString("matplotlib", "__file__")));
    this->SetMatplotlibVersion(getModuleAttrAsString("matplotlib", "__version__"));
  }

  vtkOutputWindow::SetInstance(oldWindow);
#else
  this->PythonSupportOff();
  this->SetPythonPath("");
  this->SetPythonVersion("");

  this->NumpySupportOff();
  this->SetNumpyPath("");
  this->SetNumpyVersion("");

  this->MatplotlibSupportOff();
  this->SetMatplotlibPath("");
  this->SetMatplotlibVersion("");
#endif
}

//----------------------------------------------------------------------------
void vtkPVPythonInformation::AddInformation(vtkPVInformation* i)
{
  if (vtkPVPythonInformation* pyInfo = vtkPVPythonInformation::SafeDownCast(i))
  {
    this->DeepCopy(pyInfo);
  }
}

//----------------------------------------------------------------------------
void vtkPVPythonInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->PythonSupport << this->PythonPath
       << this->PythonVersion << this->NumpySupport << this->NumpyPath << this->NumpyVersion
       << this->MatplotlibSupport << this->MatplotlibPath << this->MatplotlibVersion
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVPythonInformation::CopyFromStream(const vtkClientServerStream* css)
{
#define PARSE_NEXT_VALUE(_ivarName)                                                                \
  if (!css->GetArgument(0, i++, &this->_ivarName))                                                 \
  {                                                                                                \
    vtkErrorMacro("Error parsing " #_ivarName " from message.");                                   \
    return;                                                                                        \
  }

  int i = 0;
  PARSE_NEXT_VALUE(PythonSupport);
  PARSE_NEXT_VALUE(PythonPath);
  PARSE_NEXT_VALUE(PythonVersion);
  PARSE_NEXT_VALUE(NumpySupport);
  PARSE_NEXT_VALUE(NumpyPath);
  PARSE_NEXT_VALUE(NumpyVersion);
  PARSE_NEXT_VALUE(MatplotlibSupport);
  PARSE_NEXT_VALUE(MatplotlibPath);
  PARSE_NEXT_VALUE(MatplotlibVersion);

  this->Modified();
#undef PARSE_NEXT_VALUE
}
