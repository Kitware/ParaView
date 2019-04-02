/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWebExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkPVWebExporterModule.h" // needed for exports

class VTKPVWEBEXPORTER_EXPORT vtkPVWebExporter : public vtkJSONSceneExporter
{
public:
  static vtkPVWebExporter* New();
  vtkTypeMacro(vtkPVWebExporter, vtkJSONSceneExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of the ParaViewGlance HTML file to use to embed the data in
   */
  vtkSetStringMacro(ParaViewGlanceHTML);
  vtkGetStringMacro(ParaViewGlanceHTML);
  //@}

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
