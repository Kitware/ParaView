/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiDisplayRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiDisplayRenderModuleProxy - For tile displays
// without IceT.

#ifndef __vtkSMMultiDisplayRenderModuleProxy_h
#define __vtkSMMultiDisplayRenderModuleProxy_h

#include "vtkSMCompositeRenderModuleProxy.h"

class VTK_EXPORT vtkSMMultiDisplayRenderModuleProxy : public vtkSMCompositeRenderModuleProxy
{
public:
  static vtkSMMultiDisplayRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMMultiDisplayRenderModuleProxy, vtkSMCompositeRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMMultiDisplayRenderModuleProxy();
  ~vtkSMMultiDisplayRenderModuleProxy();


  // Description:
  // Subclasses must decide what type of CompositeManagerProxy they need.
  // This method is called to make that decision. Subclasses are expected to
  // add the CompositeManagerProxy as a SubProxy named "CompositeManager".
  virtual void CreateCompositeManager();

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  //  int UseCompositeCompression; can be direcly set on SubProxy property.
private:
  vtkSMMultiDisplayRenderModuleProxy(const vtkSMMultiDisplayRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMMultiDisplayRenderModuleProxy&); // Not implemented.
};
#endif
