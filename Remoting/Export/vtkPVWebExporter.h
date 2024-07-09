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
   * Specify file name of the custom ParaViewGlance HTML file to use to embed the data in.
   */
  vtkSetMacro(ParaViewGlanceHTML, std::string);
  vtkGetMacro(ParaViewGlanceHTML, std::string);
  ///@}

  ///@{
  /**
   * In addition to writing the scene as a .vtkjs file, bundle its content encoded in base64 into a
   * Kitware Glance viewer standalone HTML file.
   * Defaults to true.
   */
  vtkGetMacro(ExportToGlance, bool);
  vtkSetMacro(ExportToGlance, bool);
  ///@}

  ///@{
  /**
   * Ignore ParaViewGlanceHTML file path and try to find the ParaViewGlance.html file coming
   * with the ParaView distribution. This file is supposed to be in
   * [PARAVIEW ROOT]/share/paraview-[VERSION]/web/glance/ParaViewGlance.html
   * Defaults to true.
   */
  vtkGetMacro(AutomaticGlanceHTML, bool);
  vtkSetMacro(AutomaticGlanceHTML, bool);
  ///@}

  ///@{
  /**
   * Disable components requiring network access through Girder in the Glance application, by
   * forcing the URL parameter 'noGirder'. This will inject a script reloading the page if the
   * 'noGirder' parameter is not set, effectively preventing any external network call from the
   * Glance webapp. Defaults to true (Girder disabled).
   */
  vtkGetMacro(DisableNetwork, bool);
  vtkSetMacro(DisableNetwork, bool);
  ///@}

protected:
  vtkPVWebExporter();
  ~vtkPVWebExporter() override;

  // Decorate method to enable zip bundling
  void Write() override;

private:
  vtkPVWebExporter(const vtkPVWebExporter&) = delete;
  void operator=(const vtkPVWebExporter&) = delete;

  // Exporter properties
  std::string ParaViewGlanceHTML;
  bool AutomaticGlanceHTML = true;
  bool ExportToGlance = false;
  bool DisableNetwork = true;
};

#endif
