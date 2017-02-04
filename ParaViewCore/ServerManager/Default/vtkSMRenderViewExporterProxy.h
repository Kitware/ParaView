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

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMExporterProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMRenderViewExporterProxy : public vtkSMExporterProxy
{
public:
  static vtkSMRenderViewExporterProxy* New();
  vtkTypeMacro(vtkSMRenderViewExporterProxy, vtkSMExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Exports the view.
   */
  virtual void Write() VTK_OVERRIDE;

  /**
   * Returns if the view can be exported.
   * Default implementation return true if the view is a render view.
   */
  virtual bool CanExport(vtkSMProxy*) VTK_OVERRIDE;

protected:
  vtkSMRenderViewExporterProxy();
  ~vtkSMRenderViewExporterProxy();

private:
  vtkSMRenderViewExporterProxy(const vtkSMRenderViewExporterProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMRenderViewExporterProxy&) VTK_DELETE_FUNCTION;
};

#endif
