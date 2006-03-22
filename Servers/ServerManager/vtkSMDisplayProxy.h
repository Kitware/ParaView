/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplayProxy - proxy for any entity that must be rendered.
// .SECTION Description
// vtkSMDisplayProxy is a sink for display objects. Anything that can
// be rendered has to be a vtkSMDisplayProxy, otherwise it can't be added
// be added to the vtkSMRenderModule, and hence cannot be rendered.
// This can have inputs (but not required, for displays such as 3Dwidgets/ Scalarbar).
// This is an abstract class, merely defining the interface.
//  This class (or subclasses) has a bunch of 
// "convenience methods" (method names appended with CM). These methods
// do the equivalent of getting the property by the name and
// setting/getting its value. They are there to simplify using the property
// interface for display objects. When adding a method to the proxies
// that merely sets some property on the proxy, make sure to append the method
// name with "CM" - implying it's a convenience method. That way, one knows
// its purpose and will not be confused with a update-self property method.

#ifndef __vtkSMDisplayProxy_h
#define __vtkSMDisplayProxy_h

#include "vtkSMProxy.h"
class vtkSMRenderModuleProxy;
class vtkPVGeometryInformation;

class VTK_EXPORT vtkSMDisplayProxy : public vtkSMProxy
{
public:
  static vtkSMDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMDisplayProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get information about the geometry.
  // Some displays (like Scalar bar, 3DWidgets), may return NULL.
  virtual vtkPVGeometryInformation* GetGeometryInformation() { return NULL; }
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  // Default implementation searches for a subproxies with name
  // Prop/Prop2D. If found, they are added/removed to/from the 
  // Renderer/2DRenderer respectively. 
  // If such subproxies are not found no error is raised.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  virtual void Update() { }

  // Description:
  // When doing an ordered composite, some displays will need to run extra
  // filters that redistribute or clip their data before a render occurs.
  // This method makes sure that the distributed data is up to date.  The
  // default implementation just calls Update().
  virtual void UpdateDistributedGeometry() { this->Update(); }
  
  // Description:
  // Convenience method to get/set Visibility property.
  void SetVisibilityCM(int v);
  int GetVisibilityCM(); 

  // Description:
  // Save the display in batch script. This will eventually get 
  // removed as we will generate batch script from ServerManager
  // state. However, until then.
  virtual void SaveInBatchScript(ofstream* file);
  
protected:
  vtkSMDisplayProxy();
  ~vtkSMDisplayProxy();

  vtkSMProxy* GetInteractorProxy(vtkSMRenderModuleProxy* ren);
  vtkSMProxy* GetRendererProxy(vtkSMRenderModuleProxy* ren);
  vtkSMProxy* GetRenderer2DProxy(vtkSMRenderModuleProxy* ren);
  void AddPropToRenderer(vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren);
  void AddPropToRenderer2D(vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren);
  void RemovePropFromRenderer(vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren);
  void RemovePropFromRenderer2D(vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren);

private:
  vtkSMDisplayProxy(const vtkSMDisplayProxy&); // Not implemented.
  void operator=(const vtkSMDisplayProxy&); // Not implemented.
};



#endif

