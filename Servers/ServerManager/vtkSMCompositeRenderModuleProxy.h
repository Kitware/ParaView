/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeRenderModuleProxy -  Render module supporting LODs.
// .SECTION Description
// This render manager is for parallel execution using MPI.
// It creates a special vtkPVPartDisplay (todo) that collects small
// geometry for local rendering.  It also manages reduction factor
// which renders and composites a small window then magnifies for final
// display.

#ifndef __vtkSMCompositeRenderModuleProxy_h
#define __vtkSMCompositeRenderModuleProxy_h

#include "vtkSMClientServerRenderModuleProxy.h"

class vtkSMDisplayProxy;

class VTK_EXPORT vtkSMCompositeRenderModuleProxy : public vtkSMClientServerRenderModuleProxy
{
public:
  vtkTypeRevisionMacro(vtkSMCompositeRenderModuleProxy, vtkSMClientServerRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is an alias for RemoteRenderThreshold (here for backward
  // compatibility).
  void SetCompositeThreshold(double val) {this->SetRemoteRenderThreshold(val);}
  double GetCompositeThreshold() {return this->GetRemoteRenderThreshold();}

protected:
  vtkSMCompositeRenderModuleProxy();
  ~vtkSMCompositeRenderModuleProxy();

  virtual void CreateVTKObjects();

  // Description:
  // Subclasses should override this method to setup any compositing classes.
  // This is called during CreateVTKObjects();
  virtual void InitializeCompositingPipeline() { }
private:
  vtkSMCompositeRenderModuleProxy(const vtkSMCompositeRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMCompositeRenderModuleProxy&); // Not implemented.
};

#endif

