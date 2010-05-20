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
#include "vtkStdString.h" // needed for vtkStdString.
class vtkSMRenderViewProxy;

class VTK_EXPORT vtkSMTwoDRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMTwoDRenderViewProxy* New();
  vtkTypeMacro(vtkSMTwoDRenderViewProxy, vtkSMViewProxy);
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

  // Description:
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. When
  // user requests the creation of a view class, the application can call this
  // method on a prototype instantaiated for the requested class and the
  // determine the actual xmlname for the view to create.
  // Overridden to choose the correct type of render view.
  virtual const char* GetSuggestedViewType(vtkIdType connectionID);

//BTX
protected:
  vtkSMTwoDRenderViewProxy();
  ~vtkSMTwoDRenderViewProxy();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();
  virtual void EndCreateVTKObjects();

  vtkSMProxy* LegendScaleActor;
  vtkSMRenderViewProxy* RenderView;
  vtkStdString SuggestedViewType;

private:
  vtkSMTwoDRenderViewProxy(const vtkSMTwoDRenderViewProxy&); // Not implemented
  void operator=(const vtkSMTwoDRenderViewProxy&); // Not implemented
//ETX
};

#endif

