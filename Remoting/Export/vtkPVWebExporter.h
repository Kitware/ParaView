// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVWebExporter
 * @brief   vtkPVWebExporter is used to produce vtkjs export in the ParaView
 * application.
 *
 * vtkPVWebExporter is used to produce scene export for web sharing.
 */

#ifndef vtkPVWebExporter_h
#define vtkPVWebExporter_h

#include "vtkJSONSceneExporter.h"
#include "vtkRemotingExportModule.h" // needed for exports

class VTKREMOTINGEXPORT_EXPORT vtkPVWebExporter : public vtkJSONSceneExporter
{
public:
  static vtkPVWebExporter* New();
  vtkTypeMacro(vtkPVWebExporter, vtkJSONSceneExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of the ParaViewGlance HTML file to use to embed the data in
   */
  vtkSetStringMacro(ParaViewGlanceHTML);
  vtkGetStringMacro(ParaViewGlanceHTML);
  ///@}

protected:
  vtkPVWebExporter();
  ~vtkPVWebExporter() override;

  // Decorate method to enable zip bundling
  void Write() override;

private:
  vtkPVWebExporter(const vtkPVWebExporter&) = delete;
  void operator=(const vtkPVWebExporter&) = delete;

  char* ParaViewGlanceHTML;
};

#endif
