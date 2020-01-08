/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGL2PSExporterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMGL2PSExporterProxy
 * @brief   Proxy for vtkPVGL2PSExporter
 *
 *  Proxy for vtkPVGL2PSExporter
*/

#ifndef vtkSMGL2PSExporterProxy_h
#define vtkSMGL2PSExporterProxy_h

#include "vtkRemotingExportModule.h" //needed for exports
#include "vtkSMRenderViewExporterProxy.h"

class VTKREMOTINGEXPORT_EXPORT vtkSMGL2PSExporterProxy : public vtkSMRenderViewExporterProxy
{
public:
  static vtkSMGL2PSExporterProxy* New();
  vtkTypeMacro(vtkSMGL2PSExporterProxy, vtkSMRenderViewExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns if the view can be exported.
   * Default implementation return true if the view is a render view or a
   * context view.
   */
  bool CanExport(vtkSMProxy*) override;

  /**
   * Export the current view.
   */
  void Write() override;

  /**
   * See superclass documentation for description.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

protected:
  vtkSMGL2PSExporterProxy();
  ~vtkSMGL2PSExporterProxy() override;

  //@{
  /**
   * Type of view that this exporter is configured to export.
   */
  enum
  {
    None,
    ContextView,
    RenderView
  };
  int ViewType;
  //@}

private:
  vtkSMGL2PSExporterProxy(const vtkSMGL2PSExporterProxy&) = delete;
  void operator=(const vtkSMGL2PSExporterProxy&) = delete;
};

#endif
