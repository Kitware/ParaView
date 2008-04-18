/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTwoDRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTwoDRenderViewProxy
// .SECTION Description
//

#ifndef __vtkSMTwoDRenderViewProxy_h
#define __vtkSMTwoDRenderViewProxy_h

#include "vtkSMViewProxy.h"

class vtkSMRenderViewProxy;

class VTK_EXPORT vtkSMTwoDRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMTwoDRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMTwoDRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forwards the call to internal view.
  virtual void AddRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Forwards the call to internal view.
  virtual void RemoveRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Forwards the call to internal view.
  virtual void RemoveAllRepresentations();

  // Description:
  // Forwards the call to internal view.
  virtual void StillRender();

  // Description:
  // Forwards the call to internal view.
  virtual void InteractiveRender();

  // Description:
  // Forwards the call to internal view.
  virtual void SetViewUpdateTime(double time);

  // Description:
  // Forwards the call to internal view.
  virtual void SetCacheTime(double time);

  // Description:
  // Forwards the call to internal view.
  virtual void SetUseCache(int);

  // Description:
  // Forwards the call to internal view.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy* proxy)
    { return this->Superclass::CreateDefaultRepresentation(proxy); }

  // Description:
  // Forwards the call to internal view.
  virtual void SetViewPosition(int x, int y);
  virtual void SetViewPosition(int xy[2])
    { this->Superclass::SetViewPosition(xy); }

  // Description:
  // Forwards the call to internal view.
  virtual void SetGUISize(int x, int y);
  virtual void SetGUISize(int xy[2])
    { this->Superclass::SetGUISize(xy); }

  vtkGetObjectMacro(RenderView, vtkSMRenderViewProxy);
//BTX
protected:
  vtkSMTwoDRenderViewProxy();
  ~vtkSMTwoDRenderViewProxy();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();

  vtkSMRenderViewProxy* RenderView;

private:
  vtkSMTwoDRenderViewProxy(const vtkSMTwoDRenderViewProxy&); // Not implemented
  void operator=(const vtkSMTwoDRenderViewProxy&); // Not implemented
//ETX
};

#endif

