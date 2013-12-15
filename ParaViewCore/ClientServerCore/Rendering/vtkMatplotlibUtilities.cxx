/*=========================================================================

  Program:   ParaView
  Module:    vtkMatplotlibUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be the first thing that's included.
#include "vtkMatplotlibUtilities.h"

#include <vtksys/SystemTools.hxx>

#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPythonInterpreter.h"
#include "vtkObjectFactory.h"

#include <vtksys/ios/sstream>

namespace {

// Smart pointer for PyObjects. Calls Py_XDECREF when scope ends.
class SmartPyObject
{
  PyObject *Object;

public:
  SmartPyObject(PyObject *obj = NULL)
    : Object(obj)
  {
  }

  ~SmartPyObject()
  {
    Py_XDECREF(this->Object);
  }
  PyObject *operator->() const
  {
    return this->Object;
  }

  PyObject *GetPointer() const
  {
    return this->Object;
  }
};

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkMatplotlibUtilities::Availability
vtkMatplotlibUtilities::MatplotlibAvailable =
vtkMatplotlibUtilities::NOT_TESTED;

// A macro that is used in New() to print warnings if VTK_MATPLOTLIB_DEBUG
// is defined in the environment. Use vtkGenericWarningMacro to allow this to
// work in release mode builds.
#define vtkMplStartUpDebugMacro(x) if(debug){vtkGenericWarningMacro(x);}

namespace {

  //----------------------------------------------------------------------------
  // Used to replace "\ " with " " in paths.
  void UnEscapeSpaces(std::string &str)
    {
    size_t pos = str.rfind("\\ ");
    while (pos != std::string::npos)
      {
      str.erase(pos, 1);
      pos = str.rfind("\\ ", pos);
      }
    }

} // end anon namespace

//----------------------------------------------------------------------------
void vtkMatplotlibUtilities::CheckMatplotlibAvailability()
{
  if (vtkMatplotlibUtilities::MatplotlibAvailable != NOT_TESTED)
    {
    // Already tested. Nothing to do now.
    return;
    }

  // Enable startup debugging output. This will be set to true when
  // VTK_MATPLOTLIB_DEBUG is defined in the process environment.
  bool debug = (vtksys::SystemTools::GetEnv("VTK_MATPLOTLIB_DEBUG") != NULL);

  // Initialize the python interpretor if needed
  vtkMplStartUpDebugMacro("Initializing Python, if not already.");
  vtkPythonInterpreter::Initialize();
  vtkMplStartUpDebugMacro("Attempting to import matplotlib.");
  if (PyErr_Occurred() || !PyImport_ImportModule("matplotlib") || PyErr_Occurred())
    {
    // FIXME: Check if we need this. Wouldn't pipe-ing the stdout/stderr make
    // this unnecessary?

    // Fetch the exception info. Note that value and traceback may still be
    // NULL after the call to PyErr_Fetch().
    PyObject *type = NULL;
    PyObject *value = NULL;
    PyObject *traceback = NULL;
    PyErr_Fetch(&type, &value, &traceback);
    SmartPyObject typeStr(PyObject_Str(type));
    SmartPyObject valueStr(PyObject_Str(value));
    SmartPyObject tracebackStr(PyObject_Str(traceback));
    vtkMplStartUpDebugMacro(
      "Error during matplotlib import:\n"
      << "\nStack:\n"
      << (tracebackStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(tracebackStr.GetPointer())))
      << "\nValue:\n"
      << (valueStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(valueStr.GetPointer())))
      << "\nType:\n"
      << (typeStr.GetPointer() == NULL
        ? "(none)"
        : const_cast<char*>(
          PyString_AsString(typeStr.GetPointer()))));
    PyErr_Clear();
    vtkMatplotlibUtilities::MatplotlibAvailable = UNAVAILABLE;
    }
  else
    {
    vtkMplStartUpDebugMacro("Successfully imported matplotlib.");
    vtkMatplotlibUtilities::MatplotlibAvailable = AVAILABLE;
    }
}

//----------------------------------------------------------------------------
bool vtkMatplotlibUtilities::InitializeMatplotlib()
{
  // ensure that Python is initialized.
  vtkPythonInterpreter::Initialize();

  return true;
}

//----------------------------------------------------------------------------
void vtkMatplotlibUtilities::InitializeCanvas(const char* canvasName)
{
  this->DestroyCanvas(canvasName);

  vtksys_ios::ostringstream stream;
  stream << "from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas" << endl
         << "from matplotlib.figure import Figure" << endl
         << endl
         << canvasName << "Figure = Figure()" << endl
         << canvasName << " = FigureCanvas(" << canvasName << "Figure)" << endl;
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  this->CheckForPythonError();
}

//----------------------------------------------------------------------------
void vtkMatplotlibUtilities::DestroyCanvas(const char* canvasName)
{
  // Try to delete the canvas and figure variables if possible
  vtksys_ios::ostringstream stream;
  stream << "try:" << endl
         << "  del " << canvasName << endl
         << "except NameError:" << endl
         << "  pass" << endl
         << "try:" << endl
         << "  del " << canvasName << "Figure" << endl
         << "except NameError:" << endl
         << "  pass" << endl;
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
  this->CheckForPythonError();
}

//----------------------------------------------------------------------------
bool vtkMatplotlibUtilities::DetermineDPI()
{
  if (this->DPI == 0) // Unintialized
    {
    this->InitializeMatplotlib();

    this->InitializeCanvas("dpiCanvas");

    vtksys_ios::ostringstream dpiScriptInitialize;
    dpiScriptInitialize << "dpiCanvasFigure.set_size_inches(1,1)" << endl;

    vtkPythonInterpreter::RunSimpleString(dpiScriptInitialize.str().c_str());

    vtkImageData* tmpImage = this->ImageFromCanvas("dpiCanvas");
    
    this->DestroyCanvas("dpiCanvas");
    if (!tmpImage)
      {
      return false;
      }

    this->DPI = tmpImage->GetDimensions()[0];
    tmpImage->Delete();
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkMatplotlibUtilities::CleanupPythonObjects()
{
}

//----------------------------------------------------------------------------
bool vtkMatplotlibUtilities::CheckForPythonError(PyObject *object)
{
  // Print any exceptions
  bool result = this->CheckForPythonError();

  if (object == NULL)
    {
    vtkDebugMacro(<< "Object is NULL!");
    return true;
    }
  return result;
}

//----------------------------------------------------------------------------
bool vtkMatplotlibUtilities::CheckForPythonError()
{
  PyObject *exception = PyErr_Occurred();
  if (exception)
    {
    if (this->Debug)
      {
      // Fetch the exception info. Note that value and traceback may still be
      // NULL after the call to PyErr_Fetch().
      PyObject *type = NULL;
      PyObject *value = NULL;
      PyObject *traceback = NULL;
      PyErr_Fetch(&type, &value, &traceback);
      SmartPyObject typeStr(PyObject_Str(type));
      SmartPyObject valueStr(PyObject_Str(value));
      SmartPyObject tracebackStr(PyObject_Str(traceback));
      vtkWarningMacro(<< "Python exception raised:\n"
                      << "\nStack:\n"
                      << (tracebackStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(tracebackStr.GetPointer())))
                      << "\nValue:\n"
                      << (valueStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(valueStr.GetPointer())))
                      << "\nType:\n"
                      << (typeStr.GetPointer() == NULL
                          ? "(none)"
                          : const_cast<char*>(
                            PyString_AsString(typeStr.GetPointer()))));
      }
    PyErr_Clear();
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
vtkImageData* vtkMatplotlibUtilities::ImageFromCanvas(const char* canvasName)
{
  PyObject* mainModule = PyImport_AddModule("__main__");
  if ( this->CheckForPythonError(mainModule))
    {
    vtkErrorMacro(<< "Could not get __main__ module");
    return NULL;
    }

  PyObject* mainDict = PyModule_GetDict(mainModule);
  if (this->CheckForPythonError(mainDict))
    {
    vtkErrorMacro(<< "Could not get global dictionary from main module");
    return NULL;
    }

  PyObject* canvasObject = PyDict_GetItemString(mainDict, canvasName);
  if (this->CheckForPythonError(canvasObject))
    {
    vtkErrorMacro(<< "No Python canvas named '" << canvasName << "' defined in script");
    return NULL;
    }

  char printToBufferString[] = "print_to_buffer";
  SmartPyObject bufferTuple(
    PyObject_CallMethod(canvasObject, printToBufferString, NULL));
  if (this->CheckForPythonError(bufferTuple.GetPointer()))
    {
    vtkErrorMacro(<< "Could not call '" << canvasName << ".print_to_buffer()'");
    return NULL;
    }

  PyObject* bufferObject = PyTuple_GetItem(bufferTuple.GetPointer(), 0);
  if (this->CheckForPythonError(bufferObject))
    {
    vtkErrorMacro(<< "Could not get buffer object from buffer tuple");
    return NULL;
    }

  if (PyObject_CheckBuffer(bufferObject) == 0)
    {
    vtkErrorMacro(<< "Not a buffer object");
    return NULL;
    }

  // Get width and size from the buffer tuple
  PyObject* sizeTuple = PyTuple_GetItem(bufferTuple.GetPointer(), 1);
  if (this->CheckForPythonError(sizeTuple))
    {
    vtkErrorMacro(<< "Could not get size tuple from buffer tuple");
    return NULL;
    }

  PyObject* widthObject = PyTuple_GetItem(sizeTuple, 0);
  if (this->CheckForPythonError(widthObject))
    {
    vtkErrorMacro(<< "Could not get width from size tuple");
    return NULL;
    }

  if (PyInt_Check(widthObject) == 0)
    {
    vtkErrorMacro(<< "Expected width is not an int");
    return NULL;
    }

  long width = PyInt_AsLong(widthObject);
  if (this->CheckForPythonError())
    {
    vtkErrorMacro(<< "Error accessing width as int");
    return NULL;
    }

  PyObject* heightObject = PyTuple_GetItem(sizeTuple, 1);
  if (this->CheckForPythonError(heightObject))
    {
    vtkErrorMacro(<< "Could not get height from size tuple");
    return NULL;
    }

  if (PyInt_Check(heightObject) == 0)
    {
    vtkErrorMacro(<< "Expected height is not an int");
    return NULL;
    }

  long height = PyInt_AsLong(heightObject);
  if (this->CheckForPythonError())
    {
    vtkErrorMacro(<< "Error accessing height as int");
    return NULL;
    }

  if (width == -1 || height == -1)
    {
    vtkErrorMacro(<< "Invalid canvas size (" << width << ", " << height << ")");
    return NULL;
    }

  vtkImageData* image = vtkImageData::New();
  image->SetDimensions(width, height, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  unsigned char* dst = static_cast<unsigned char*>(image->GetScalarPointer());
  for (int j = 0; j < height; ++j)
    {
    for (int i = 0; i < width; ++i)
      {
      dst[4*i + 0] = 0;
      dst[4*i + 1] = 0;
      dst[4*i + 2] = 0;
      dst[4*i + 3] = 0;
      }
    }

  Py_buffer bufferView;
  if (PyObject_GetBuffer(bufferObject, &bufferView, PyBUF_C_CONTIGUOUS) == -1)
    {
    vtkErrorMacro(<< "Could not get view of buffer");
    return image;
    }

  // Rows are flipped in the bufferView.buf
  int rowWidth = 4*width*sizeof(unsigned char);
  for (int row = 0; row < height; ++row)
    {
    unsigned char* src = static_cast<unsigned char*>(bufferView.buf);
    memcpy(dst + (rowWidth*row), src + (rowWidth*(height-row-1)), rowWidth);
    }

  return image;
}

//----------------------------------------------------------------------------
vtkMatplotlibUtilities* vtkMatplotlibUtilities::New()
{
  vtkMatplotlibUtilities::CheckMatplotlibAvailability();

  // Attempt to import matplotlib to check for availability
  switch (vtkMatplotlibUtilities::MatplotlibAvailable)
    {
  case vtkMatplotlibUtilities::AVAILABLE:
    break;

  case vtkMatplotlibUtilities::NOT_TESTED:
  case vtkMatplotlibUtilities::UNAVAILABLE:
  default:
    return NULL;
    }

  // Adapted from VTK_OBJECT_FACTORY_NEW_BODY to enable debugging output when
  // requested.
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkMatplotlibUtilities");
  if (ret)
    {
    return static_cast<vtkMatplotlibUtilities*>(ret);
    }

  return new vtkMatplotlibUtilities;
}
vtkInstantiatorNewMacro(vtkMatplotlibUtilities)
//----------------------------------------------------------------------------
vtkMatplotlibUtilities::vtkMatplotlibUtilities()
  : Superclass()
{
  this->Interpreter = vtkPythonInterpreter::New();
  this->Interpreter->AddObserver(vtkCommand::ExitEvent,
    this, &vtkMatplotlibUtilities::CleanupPythonObjects);

  this->DPI = 0;
}

//----------------------------------------------------------------------------
vtkMatplotlibUtilities::~vtkMatplotlibUtilities()
{
  this->CleanupPythonObjects();
  this->Interpreter->Delete();
}

//----------------------------------------------------------------------------
vtkImageData* vtkMatplotlibUtilities::ImageFromScript(const char* script,
                                                      const char* canvasName,
                                                      int width, int height)
{
  if (!script || !canvasName)
    {
    return NULL;
    }

  this->DetermineDPI();

  // Initialize canvas and figure
  this->InitializeCanvas(canvasName);

  vtkPythonInterpreter::RunSimpleString(script);

  // Resize the canvas to match the requested size of the image parameter
  float fDPI = static_cast<float>(this->DPI);
  char commandBuffer[256];
  sprintf(commandBuffer, "%sFigure.set_size_inches(%f, %f, forward=True)",
          canvasName, width/fDPI, height/fDPI);
  vtkPythonInterpreter::RunSimpleString(commandBuffer);

  vtkImageData* image = this->ImageFromCanvas(canvasName);

  this->DestroyCanvas(canvasName);

  return image;
}

//----------------------------------------------------------------------------
void vtkMatplotlibUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << endl;
}
