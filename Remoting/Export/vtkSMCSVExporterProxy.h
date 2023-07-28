// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMCSVExporterProxy
 * @brief   exporter used to export the spreadsheet view
 * and supported chart views as to a CSV file.
 *
 * vtkSMCSVExporterProxy is used to export the certain views to a CSV file.
 * Currently, we support vtkSpreadSheetView and vtkPVXYChartView (which includes
 * Bar/Line/Quartile/Parallel Coordinates views).
 *
 * @sa vtkCSVExporter
 */

#ifndef vtkSMCSVExporterProxy_h
#define vtkSMCSVExporterProxy_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMExporterProxy.h"

class VTKREMOTINGEXPORT_EXPORT vtkSMCSVExporterProxy : public vtkSMExporterProxy
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
