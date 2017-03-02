/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExporterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMExporterProxy
 * @brief   proxy for view exporters.
 *
 * vtkSMExporterProxy is a proxy for vtkExporter subclasses. It makes it
 * possible to export render views using these exporters.
*/

#ifndef vtkSMExporterProxy_h
#define vtkSMExporterProxy_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMViewProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMExporterProxy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMExporterProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the view proxy to export.
   */
  void SetView(vtkSMViewProxy* view);
  vtkGetObjectMacro(View, vtkSMViewProxy);
  //@}

  /**
   * Exports the view.
   */
  virtual void Write() = 0;

  /**
   * Returns if the view can be exported.
   * Default implementation return true if the view is a render view.
   */
  virtual bool CanExport(vtkSMProxy*) = 0;

  //@{
  /**
   * Returns the suggested file extension for this exporter.
   */
  vtkGetStringMacro(FileExtension);
  //@}

protected:
  vtkSMExporterProxy();
  ~vtkSMExporterProxy();
  /**
   * Read attributes from an XML element.
   */
  virtual int ReadXMLAttributes(
    vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) VTK_OVERRIDE;

  vtkSetStringMacro(FileExtension);
  vtkSMViewProxy* View;
  char* FileExtension;

private:
  vtkSMExporterProxy(const vtkSMExporterProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMExporterProxy&) VTK_DELETE_FUNCTION;
};

#endif
