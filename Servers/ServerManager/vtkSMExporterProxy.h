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
// .NAME vtkSMExporterProxy - proxy for vtkExporter subclasses.
// .SECTION Description
// vtkSMExporterProxy is a proxy for vtkExporter subclasses. It makes it
// possible to export render views using these exporters.

#ifndef __vtkSMExporterProxy_h
#define __vtkSMExporterProxy_h

#include "vtkSMProxy.h"

class vtkSMRenderViewProxy;

class VTK_EXPORT vtkSMExporterProxy : public vtkSMProxy
{
public:
  static vtkSMExporterProxy* New();
  vtkTypeRevisionMacro(vtkSMExporterProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the view proxy to export.
  void SetView(vtkSMRenderViewProxy* view);
  vtkGetObjectMacro(View, vtkSMRenderViewProxy);

  // Description:
  // Exports the view.
  virtual void Write();

  // Description:
  // Returns if the view can be exported. 
  // Default implementation return true if the view is a render view.
  virtual bool CanExport(vtkSMProxy*);

  // Description:
  // Returns the suggested file extension for this exporter.
  vtkGetStringMacro(FileExtension);
//BTX
protected:
  vtkSMExporterProxy();
  ~vtkSMExporterProxy();
  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  vtkSetStringMacro(FileExtension);
  vtkSMRenderViewProxy* View;
  char* FileExtension;
private:
  vtkSMExporterProxy(const vtkSMExporterProxy&); // Not implemented
  void operator=(const vtkSMExporterProxy&); // Not implemented
//ETX
};

#endif

