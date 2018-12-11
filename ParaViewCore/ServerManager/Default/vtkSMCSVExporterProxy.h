/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCSVExporterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCSVExporterProxy
 * @brief   exporter used to export the spreadsheet view
 * and supported chart views as to a CSV file.
 *
 * vtkSMCSVExporterProxy is used to export the certain views to a CSV file.
 * Currently, we support vtkSpreadSheetView and vtkPVXYChartView (which includes
 * Bar/Line/Quartile/Parallel Coordinates views).
*/

#ifndef vtkSMCSVExporterProxy_h
#define vtkSMCSVExporterProxy_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMExporterProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMCSVExporterProxy : public vtkSMExporterProxy
{
public:
  static vtkSMCSVExporterProxy* New();
  vtkTypeMacro(vtkSMCSVExporterProxy, vtkSMExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Exports the view.
   */
  void Write() override;

  /**
   * Returns if the view can be exported.
   * Default implementation return true if the view is a render view.
   */
  bool CanExport(vtkSMProxy*) override;

protected:
  vtkSMCSVExporterProxy();
  ~vtkSMCSVExporterProxy() override;

private:
  vtkSMCSVExporterProxy(const vtkSMCSVExporterProxy&) = delete;
  void operator=(const vtkSMCSVExporterProxy&) = delete;
};

#endif
