/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScatterPlotViewProxy - view for scatter plot.
// .SECTION Description
// vtkSMScatterPlotViewProxy is the view used to generate/scatter plots

#ifndef __vtkSMScatterPlotViewProxy_h
#define __vtkSMScatterPlotViewProxy_h

#include "vtkSMRenderViewProxy.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkSMRenderViewProxy;
class vtkEventForwarderCommand;

class VTK_EXPORT vtkSMScatterPlotViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMScatterPlotViewProxy* New();
  vtkTypeMacro(vtkSMScatterPlotViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forwards the call to internal view.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

//BTX
protected:
  vtkSMScatterPlotViewProxy();
  ~vtkSMScatterPlotViewProxy();

private:
  vtkSMScatterPlotViewProxy(const vtkSMScatterPlotViewProxy&); // Not implemented
  void operator=(const vtkSMScatterPlotViewProxy&); // Not implemented

//ETX
};

#endif
