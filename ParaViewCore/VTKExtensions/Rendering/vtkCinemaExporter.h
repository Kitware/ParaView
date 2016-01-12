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
// .NAME vtkCinemaExporter - Exports a view as a Cinema database.
//
// .SECTION Description
// Specifies and runs a Python script which uses pv_introspect.py to generate
// images from a set of parameters of the different elements in a pipeline for
// later visualization.

#ifndef vtkCinemaExporter_h
#define vtkCinemaExporter_h

#include "vtkExporter.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro


class  VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkCinemaExporter : public vtkExporter
{
public:
  static vtkCinemaExporter* New();
  vtkTypeMacro(vtkCinemaExporter,vtkExporter);

  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkCinemaExporter();
  ~vtkCinemaExporter();

  void WriteData();

  /////////////////////////////////////////////////////////////////////////////

  char* FileName;

private:
  /// @brief Defines the Python script to be called.
  char const* GetPythonScript();

  /// @brief Checks the initialization status of the Python interpreter and
  /// initializes it if required.
  bool checkInterpreterInitialization();

  /// @{
  /// @brief Not implemented
  vtkCinemaExporter(const vtkCinemaExporter&); // Not implemented
  void operator=(const vtkCinemaExporter&); // Not implemented
  /// @}
};

#endif
