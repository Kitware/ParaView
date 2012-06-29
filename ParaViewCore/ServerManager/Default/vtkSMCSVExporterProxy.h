/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCSVExporterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCSVExporterProxy - exporter used to export the spreadsheet view as
// a CSV file.
// .SECTION Description
// vtkSMCSVExporterProxy is used to export the spreadsheet view as a CSV file.

#ifndef __vtkSMCSVExporterProxy_h
#define __vtkSMCSVExporterProxy_h

#include "vtkSMExporterProxy.h"

class VTK_EXPORT vtkSMCSVExporterProxy : public vtkSMExporterProxy
{
public:
  static vtkSMCSVExporterProxy* New();
  vtkTypeMacro(vtkSMCSVExporterProxy, vtkSMExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Exports the view.
  virtual void Write();

  // Description:
  // Returns if the view can be exported. 
  // Default implementation return true if the view is a render view.
  virtual bool CanExport(vtkSMProxy*);

//BTX
protected:
  vtkSMCSVExporterProxy();
  ~vtkSMCSVExporterProxy();

private:
  vtkSMCSVExporterProxy(const vtkSMCSVExporterProxy&); // Not implemented
  void operator=(const vtkSMCSVExporterProxy&); // Not implemented
//ETX
};

#endif

