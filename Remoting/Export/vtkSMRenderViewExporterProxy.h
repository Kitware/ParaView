/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewExporterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRenderViewExporterProxy
 * @brief   proxy for vtkExporter subclasses which
 * work with render windows.
 *
 * vtkSMRenderViewExporterProxy is a proxy for vtkExporter subclasses. It makes it
 * possible to export render views using these exporters.
*/

#ifndef vtkSMRenderViewExporterProxy_h
#define vtkSMRenderViewExporterProxy_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMExporterProxy.h"

class VTKREMOTINGEXPORT_EXPORT vtkSMRenderViewExporterProxy : public vtkSMExporterProxy
{
public:
  static vtkSMRenderViewExporterProxy* New();
  vtkTypeMacro(vtkSMRenderViewExporterProxy, vtkSMExporterProxy);
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
  vtkSMRenderViewExporterProxy();
  ~vtkSMRenderViewExporterProxy() override;

private:
  vtkSMRenderViewExporterProxy(const vtkSMRenderViewExporterProxy&) = delete;
  void operator=(const vtkSMRenderViewExporterProxy&) = delete;
};

#endif
