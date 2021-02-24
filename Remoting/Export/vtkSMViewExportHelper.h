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
/**
 * @class   vtkSMViewExportHelper
 * @brief   helper class to export views.
 *
 * vtkSMViewExportHelper is a helper class to aid in exporting views. You can
 * create instances of this helper on-demand to query available exporters and
 * create and exporter proxy (in same spirit as vtkSMWriterFactory, except that
 * there's no globally existing instance).
*/

#ifndef vtkSMViewExportHelper_h
#define vtkSMViewExportHelper_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMObject.h"

#include <string> // for std::string

class vtkSMViewProxy;
class vtkSMExporterProxy;

class VTKREMOTINGEXPORT_EXPORT vtkSMViewExportHelper : public vtkSMObject
{
public:
  static vtkSMViewExportHelper* New();
  vtkTypeMacro(vtkSMViewExportHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns a formatted string with all supported file types for the given
   * view.
   * An example returned string would look like:
   * \verbatim
   * "PVD Files (*.pvd);;VTK Files (*.vtk)"
   * \endverbatim
   */
  virtual std::string GetSupportedFileTypes(vtkSMViewProxy* view);

  /**
   * Exports the view to the given output file. Returns a new exporter instance
   * (or nullptr). Caller must release the returned object explicitly.
   */
  virtual vtkSMExporterProxy* CreateExporter(const char* filename, vtkSMViewProxy*);

protected:
  vtkSMViewExportHelper();
  ~vtkSMViewExportHelper() override;

private:
  vtkSMViewExportHelper(const vtkSMViewExportHelper&) = delete;
  void operator=(const vtkSMViewExportHelper&) = delete;
};

#endif
