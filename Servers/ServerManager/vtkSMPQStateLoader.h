/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPQStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPQStateLoader - State loader for ParaQ client.
// .SECTION Description
// SMState file has render module states in it. Typically one can simply load back
// the SMState and create new rendermodules from that SMState, just like other proxies.
// However, when using MultiViewRenderModuleProxy, if a MultiViewRenderModuleProxy 
// exists, it must be used to obtain the rendermodules, otherwise they will
// not work correctly. Hence, we provide this loader. Set the MultiViewRenderModuleProxy
// on this loader before loading the state. Then, when a render module is
// encountered in the state the MultiViewRenderModuleProxy is requested to return
// a render module and the state is loaded on that render module.

#ifndef __vtkSMPQStateLoader_h
#define __vtkSMPQStateLoader_h

#include "vtkSMStateLoader.h"

class vtkSMMultiViewRenderModuleProxy;

class VTK_EXPORT vtkSMPQStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMPQStateLoader* New();
  vtkTypeRevisionMacro(vtkSMPQStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the vtkSMMultiViewRenderModuleProxy proxy to use to
  // create the render modules. This must be set before loading the state.
  vtkGetObjectMacro(MultiViewRenderModuleProxy, vtkSMMultiViewRenderModuleProxy);
  void SetMultiViewRenderModuleProxy(vtkSMMultiViewRenderModuleProxy*);

  // Description:
  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key.
  virtual vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);
protected:
  vtkSMPQStateLoader();
  ~vtkSMPQStateLoader();

  vtkSMMultiViewRenderModuleProxy* MultiViewRenderModuleProxy;
private:
  vtkSMPQStateLoader(const vtkSMPQStateLoader&); // Not implemented.
  void operator=(const vtkSMPQStateLoader&); // Not implemented.
};

#endif

