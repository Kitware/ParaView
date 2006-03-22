/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiViewRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiViewRenderModuleProxy - render module for multiple views
// .SECTION Description
// vtkMultiViewRenderModuleProxy performs all the initialization and
// maintains all the necessary proxies for multiple views. When 
// rendering locally, this is almost nothing. However, when using
// compositing (only IceT supports this), there are several server
// side objects that have to be created and maintained.
// A vtkSMRenderModule still has to be created and maintained for each view.

#ifndef __vtkMultiViewRenderModuleProxy_h
#define __vtkMultiViewRenderModuleProxy_h

#include "vtkSMCompoundProxy.h"

class VTK_EXPORT vtkSMMultiViewRenderModuleProxy : public vtkSMCompoundProxy
{
public:
  static vtkSMMultiViewRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMMultiViewRenderModuleProxy, vtkSMCompoundProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The class name of the render module create with NewRenderModule().
  // Make sure to call this right after instantiating the multiview
  // render module. Otherwise, CreateVTKObjects() and NewRenderModule()
  // will not work.
  vtkGetStringMacro(RenderModuleName);
  vtkSetStringMacro(RenderModuleName);

  // Description:
  // Returns a render module of the type appropriate for multiple views.
  // There is always one render module per view. Only local rendering
  // and IceT supports this.
  vtkSMProxy* NewRenderModule();

protected:
  vtkSMMultiViewRenderModuleProxy();
  ~vtkSMMultiViewRenderModuleProxy();

  virtual void CreateVTKObjects(int numObjects);

  char* RenderModuleName;

  int RenderModuleId;

private:
  vtkSMMultiViewRenderModuleProxy(const vtkSMMultiViewRenderModuleProxy&); // Not implemented
  void operator=(const vtkSMMultiViewRenderModuleProxy&); // Not implemented
};

#endif
