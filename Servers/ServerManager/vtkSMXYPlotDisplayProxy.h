/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXYPlotDisplayProxy - Proxy for XY Plot Display.
// .SECTION Desription
// This is the display proxy for XY Plot. It can be added to a render module
// proxy to be rendered.
// .SECTION See Also
// vtkSMXYPlotActorProxy

#ifndef __vtkSMXYPlotDisplayProxy_h
#define __vtkSMXYPlotDisplayProxy_h

#include "vtkSMDisplayProxy.h"

class VTK_EXPORT vtkSMXYPlotDisplayProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMXYPlotDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMXYPlotDisplayProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMXYPlotDisplayProxy();
  ~vtkSMXYPlotDisplayProxy();

  virtual void CreateVTKObjects(int numObjects);

  void SetupPipeline();
  void SetupDefaults();

  vtkSMProxy* XYPlotActorProxy;
  vtkSMProxy* PropertyProxy;
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* CollectProxy;

private:
  vtkSMXYPlotDisplayProxy(const vtkSMXYPlotDisplayProxy&); // Not implemented.
  void operator=(const vtkSMXYPlotDisplayProxy&); // Not implemented.
};


#endif
