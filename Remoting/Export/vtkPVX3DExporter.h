/*=========================================================================

  Program:   ParaView
  Module:    vtkPVX3DExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVX3DExporter
 * @brief   ParaView-specific X3D exporter that handles export of color legends
 *          in addition to the items exported by vtkX3DExporter.
 */

#ifndef vtkPVX3DExporter_h
#define vtkPVX3DExporter_h

#include "vtkRemotingExportModule.h" //needed for exports

#include "vtkX3DExporter.h"

// Forward declarations
class vtkContext2DScalarBarActor;
class vtkRenderer;
class vtkX3DExporterWriter;

class VTKREMOTINGEXPORT_EXPORT vtkPVX3DExporter : public vtkX3DExporter
{
public:
  static vtkPVX3DExporter* New();
  vtkTypeMacro(vtkPVX3DExporter, vtkX3DExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Indicates whether color legends should be exported.
   */
  vtkSetMacro(ExportColorLegends, bool);
  vtkGetMacro(ExportColorLegends, bool);
  vtkBooleanMacro(ExportColorLegends, bool);

protected:
  vtkPVX3DExporter();
  ~vtkPVX3DExporter() override;

  bool ExportColorLegends;

  /**
   * Write additional data, including color legends, to the output.
   */
  void WriteAdditionalNodes(vtkX3DExporterWriter* writer) override;

  /**
   * Write out color legends.
   */
  void WriteColorLegends(vtkX3DExporterWriter* writer);

  /**
   * Write out a single color legend.
   */
  void WriteColorLegend(vtkRenderer* bottomRenderer, vtkRenderer* annotationRenderer,
    vtkContext2DScalarBarActor* actor, vtkX3DExporterWriter* writer);

private:
  vtkPVX3DExporter(const vtkPVX3DExporter&) = delete;
  void operator=(const vtkPVX3DExporter&) = delete;
};

#endif
