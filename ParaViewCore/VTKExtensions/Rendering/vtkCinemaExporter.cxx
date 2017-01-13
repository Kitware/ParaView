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
#include "vtkCinemaExporter.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_ENABLE_PYTHON
#include "vtkStdString.h"
#ifdef PARAVIEW_ENABLE_PYTHON
#include "vtkPythonInterpreter.h"
#endif

vtkStandardNewMacro(vtkCinemaExporter);

vtkCinemaExporter::vtkCinemaExporter()
  : vtkExporter()
  , FileName(NULL)
  , ViewSelection(NULL)
  , TrackSelection(NULL)
  , ArraySelection(NULL)
{
}

vtkCinemaExporter::~vtkCinemaExporter()
{
  delete[] this->FileName;
  delete[] this->ViewSelection;
  delete[] this->TrackSelection;
  delete[] this->ArraySelection;
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
  int const r = vtkPythonInterpreter::RunSimpleString(this->GetPythonScript().c_str());

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
    // (1) is recommended for embedded in the documentation
    // (https://docs.python.org/2/c-api/init.html)
    vtkPythonInterpreter::Initialize(1);
    if (!vtkPythonInterpreter::IsInitialized())
    {
      vtkErrorMacro(<< "Failed to initialize vtkPythonInterpreter!");
      return false;
    }
  }

  return true;
#else
  vtkErrorMacro(<< "Export Failed! Python support is required to export a Cinema store.");
  return false;
#endif
}

const vtkStdString vtkCinemaExporter::GetPythonScript()
{
  vtkStdString script;
  script += "import paraview\n";
  script += "ready=True\n";
  script += "try:\n";
  script += "    import paraview.simple\n";
  script += "    import cinema_python.adaptors.paraview.pv_introspect as pvi\n";
  script += "except ImportError as e:\n";
  script += "    paraview.print_error('Cannot import cinema')\n";
  script += "    paraview.print_error(e)\n";
  script += "    ready=False\n";
  script += "if ready:\n";
  script += "    pvi.export_scene(baseDirName=\"";
  script += this->FileName ? this->FileName : "";
  script += "\", viewSelection={";
  script += this->ViewSelection ? this->ViewSelection : "";
  script += "}, trackSelection={";
  script += this->TrackSelection ? this->TrackSelection : "";
  script += "}, arraySelection={";
  script += this->ArraySelection ? this->ArraySelection : "";
  script += "})\n";

  return script;
}

void vtkCinemaExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  char const* fn = this->FileName ? this->FileName : "(null)";
  os << indent << "FileName: " << fn << '\n';

  char const* vs = this->ViewSelection ? this->ViewSelection : "(null)";
  os << indent << "ViewSelection: " << vs << '\n';

  char const* ts = this->TrackSelection ? this->TrackSelection : "(null)";
  os << indent << "TrackSelection: " << ts << '\n';

  char const* arr = this->ArraySelection ? this->ArraySelection : "(null)";
  os << indent << "ArraySelection: " << arr << '\n';

  os << indent << "PythonScript: " << this->GetPythonScript().c_str() << "\n";
}
