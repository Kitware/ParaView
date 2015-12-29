/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaExporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"
#include "vtkCinemaExporter.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_ENABLE_PYTHON
#ifdef PARAVIEW_ENABLE_PYTHON
#include "vtkPythonInterpreter.h"
#endif


vtkStandardNewMacro(vtkCinemaExporter);

vtkCinemaExporter::vtkCinemaExporter()
: vtkExporter()
, FileName(NULL)
{
  // Sometimes the call to RunSimpleString() does not execute the script correctly
  // right after calling Initialize(). To avoid this, Initialize() is called before
  // the first call to this->WriteData()
  this->checkInterpreterInitialization();
}

vtkCinemaExporter::~vtkCinemaExporter()
{
  delete[] this->FileName;
}

void vtkCinemaExporter::WriteData()
{
  if (!this->FileName)
    {
    vtkErrorMacro(<< "No file name was specified!");
    return;
    }

  if (!this->checkInterpreterInitialization())
    {
    return;
    }

#ifdef PARAVIEW_ENABLE_PYTHON
  int const r = vtkPythonInterpreter::RunSimpleString(this->GetPythonScript());
  if (r != 0)
    {
    vtkErrorMacro(<< "An error occurred while running the Cinema export script!");
    return;
    }
#endif
}

bool vtkCinemaExporter::checkInterpreterInitialization()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  if (!vtkPythonInterpreter::IsInitialized())
    {
    // Initialize() returns false if already initialized, so a second check is necessary.
    // (1) is recommended for embedded in the documentation (https://docs.python.org/2/c-api/init.html)
    vtkPythonInterpreter::Initialize(1);
    if (!vtkPythonInterpreter::IsInitialized())
      {
      vtkErrorMacro(<< "Failed to initialize vtkPythonInterpreter!");
      return false;
      }
    }

  return true;
#else
  return false;
#endif
}
char const* vtkCinemaExporter::GetPythonScript()
{
  std::string script;
  script += "import paraview\n";
  script += "ready=True\n";
  script += "try:\n";
  script += "    import paraview.simple\n";
  script += "    import paraview.cinemaIO.cinema_store\n";
  script += "    import paraview.cinemaIO.pv_introspect as pvi\n";
  script += "except ImportError as e:\n";
  script += "    paraview.print_error('Cannot import cinema')\n";
  script += "    paraview.print_error(e)\n";
  script += "    ready=False\n";
  script += "if ready:\n";
  script += "    pvi.record(csname=\"";
  script += this->FileName;
  script += "\")\n";

  return script.c_str();
}

void vtkCinemaExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << "\n";
    }
  else
    {
    os << indent << "FileName: (null)\n";
    }

  os << indent << "PythonScript: " << this->GetPythonScript() << "\n";
}
