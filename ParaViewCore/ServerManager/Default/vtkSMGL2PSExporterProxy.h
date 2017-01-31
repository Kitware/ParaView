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

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMRenderViewExporterProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMGL2PSExporterProxy : public vtkSMRenderViewExporterProxy
{
public:
  static vtkSMGL2PSExporterProxy* New();
  vtkTypeMacro(vtkSMGL2PSExporterProxy, vtkSMRenderViewExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns if the view can be exported.
   * Default implementation return true if the view is a render view or a
   * context view.
   */
  bool CanExport(vtkSMProxy*) VTK_OVERRIDE;

  /**
   * Export the current view.
   */
  void Write() VTK_OVERRIDE;

  /**
   * See superclass documentation for description.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) VTK_OVERRIDE;

protected:
  vtkSMGL2PSExporterProxy();
  ~vtkSMGL2PSExporterProxy();

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
  vtkSMGL2PSExporterProxy(const vtkSMGL2PSExporterProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMGL2PSExporterProxy&) VTK_DELETE_FUNCTION;
};

#endif
