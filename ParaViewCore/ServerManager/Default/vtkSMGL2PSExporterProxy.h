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
// .NAME vtkSMGL2PSExporterProxy - Proxy for vtkPVGL2PSExporter
// .SECTION Description
//  Proxy for vtkPVGL2PSExporter

#ifndef __vtkSMGL2PSExporterProxy_h
#define __vtkSMGL2PSExporterProxy_h

#include "vtkSMRenderViewExporterProxy.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMGL2PSExporterProxy :
    public vtkSMRenderViewExporterProxy
{
public:
  static vtkSMGL2PSExporterProxy* New();
  vtkTypeMacro(vtkSMGL2PSExporterProxy, vtkSMRenderViewExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if the view can be exported. 
  // Default implementation return true if the view is a render view or a
  // context view.
  bool CanExport(vtkSMProxy*);

  // Description:
  // Export the current view.
  void Write();

//BTX
protected:
  vtkSMGL2PSExporterProxy();
  ~vtkSMGL2PSExporterProxy();

private:
  vtkSMGL2PSExporterProxy(const vtkSMGL2PSExporterProxy&); // Not implemented
  void operator=(const vtkSMGL2PSExporterProxy&); // Not implemented
//ETX
};

#endif
