/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCinemaExporter
 * @brief   Exports a view as a Cinema database.
 *
 *
 * Specifies and runs a Python script which uses pv_introspect.py to generate
 * images from a set of parameters of the different elements in a pipeline for
 * later visualization. Takes different options from pqCinemaTrackSelection and
 * pqExportViewSelection as strings to be included in the script.
*/

#ifndef vtkCinemaExporter_h
#define vtkCinemaExporter_h

#include "vtkExporter.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkCinemaExporter : public vtkExporter
{
public:
  static vtkCinemaExporter* New();
  vtkTypeMacro(vtkCinemaExporter, vtkExporter);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetStringMacro(ViewSelection);
  vtkGetStringMacro(ViewSelection);

  vtkSetStringMacro(TrackSelection);
  vtkGetStringMacro(TrackSelection);

  vtkSetStringMacro(ArraySelection);
  vtkGetStringMacro(ArraySelection);

protected:
  vtkCinemaExporter();
  ~vtkCinemaExporter();

  void WriteData() VTK_OVERRIDE;

  /////////////////////////////////////////////////////////////////////////////

  char* FileName;

  char* ViewSelection;

  char* TrackSelection;

  char* ArraySelection;

private:
  /// @brief Defines the Python script to be ran.
  const vtkStdString GetPythonScript();

  /// @brief Checks the initialization status of the Python interpreter and
  /// initializes it if required.
  bool checkInterpreterInitialization();

  vtkCinemaExporter(const vtkCinemaExporter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCinemaExporter&) VTK_DELETE_FUNCTION;
};

#endif
