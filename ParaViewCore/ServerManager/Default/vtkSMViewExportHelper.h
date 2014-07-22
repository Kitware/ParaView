/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewExportHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMViewExportHelper - helper class to export views.
// .SECTION Description
// vtkSMViewExportHelper is a helper class to aid in exporting views. You can
// create instances of this helper on-demand to query available exporters and
// create and exporter proxy (in same spirit as vtkSMWriterFactory, except that
// there's no globally existing instance).

#ifndef __vtkSMViewExportHelper_h
#define __vtkSMViewExportHelper_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMObject.h"
#include "vtkStdString.h" //needed for vtkStdString.

class vtkSMViewProxy;
class vtkSMExporterProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMViewExportHelper : public vtkSMObject
{
public:
  static vtkSMViewExportHelper* New();
  vtkTypeMacro(vtkSMViewExportHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a formatted string with all supported file types for the given
  // view.
  // An example returned string would look like:
  // \verbatim
  // "PVD Files (*.pvd);;VTK Files (*.vtk)"
  // \endverbatim
  virtual vtkStdString GetSupportedFileTypes(vtkSMViewProxy* view);

  // Description:
  // Exports the view to the given output file. Returns a new exporter instance
  // (or NULL). Caller must release the returned object explicitly.
  virtual vtkSMExporterProxy* CreateExporter(const char* filename, vtkSMViewProxy*);

//BTX
protected:
  vtkSMViewExportHelper();
  ~vtkSMViewExportHelper();

private:
  vtkSMViewExportHelper(const vtkSMViewExportHelper&); // Not implemented
  void operator=(const vtkSMViewExportHelper&); // Not implemented
//ETX
};

#endif
