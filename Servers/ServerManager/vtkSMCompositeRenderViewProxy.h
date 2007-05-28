/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeRenderViewProxy -  Render module supporting LODs.
// .SECTION Description
// This render manager is for parallel execution using MPI.
// It creates a special vtkPVPartDisplay (todo) that collects small
// geometry for local rendering.  It also manages reduction factor
// which renders and composites a small window then magnifies for final
// display.

#ifndef __vtkSMCompositeRenderViewProxy_h
#define __vtkSMCompositeRenderViewProxy_h

#include "vtkSMClientServerRenderViewProxy.h"

class vtkSMDisplayProxy;

class VTK_EXPORT vtkSMCompositeRenderViewProxy : public vtkSMClientServerRenderViewProxy
{
public:
  vtkTypeRevisionMacro(vtkSMCompositeRenderViewProxy, vtkSMClientServerRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMCompositeRenderViewProxy();
  ~vtkSMCompositeRenderViewProxy();

  virtual void CreateVTKObjects();

  // Description:
  // Subclasses should override this method to setup any compositing classes.
  // This is called during CreateVTKObjects();
  virtual void InitializeCompositingPipeline()=0;
private:
  vtkSMCompositeRenderViewProxy(const vtkSMCompositeRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMCompositeRenderViewProxy&); // Not implemented.
};

#endif

