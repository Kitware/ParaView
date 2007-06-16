/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiViewFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiViewFactory - factor for multiple views.
// .SECTION Description
// vtkSMMultiViewFactory must be used to create render views in a multiview
// system. This makes it possible to share the view objects on the server side.
// One can still create individual render view views, however, they won't be
// sharing their server side render window.

#ifndef __vtkSMMultiViewFactory_h
#define __vtkSMMultiViewFactory_h

#include "vtkSMProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSMMultiViewFactory : public vtkSMProxy
{
public:
  static vtkSMMultiViewFactory* New();
  vtkTypeRevisionMacro(vtkSMMultiViewFactory, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The class name of the render view create with NewRenderView().
  // Make sure to call this right after instantiating the multiview
  // render view. Otherwise, CreateVTKObjects() and NewRenderView()
  // will not work.
  vtkGetStringMacro(RenderViewName);
  vtkSetStringMacro(RenderViewName);

  // Description:
  // Returns a render view of the type appropriate for multiple views.
  // There is always one renderview per view. Only local rendering
  // and IceT supports this.
  vtkSMViewProxy* NewRenderView();

  // Description:
  // Don't use these methods directly,
  // Whenever NewRenderView() is called, the newly created render view 
  // is added to the "RenderViews" property of this proxy. The property
  // leads to a call to these methods to add/remove the proxy which gets 
  // added/removed as a subproxy. We have a property for the render views 
  // so that the operation is undo/redo able.
  void AddRenderView(vtkSMViewProxy*);
  void RemoveRenderView(vtkSMViewProxy*);

  // Description:
  // Returns the total number of views known to this factory.
  unsigned int GetNumberOfRenderViews();
 
  // Description:
  // Returns the view at the given index.
  vtkSMViewProxy* GetRenderView(unsigned int i);
//BTX
protected:
  vtkSMMultiViewFactory();
  ~vtkSMMultiViewFactory();

  // Description:
  // Overridden to created shared server side objects based on the render view
  // type.
  virtual void CreateVTKObjects();

  char* RenderViewName;

private:
  vtkSMMultiViewFactory(const vtkSMMultiViewFactory&); // Not implemented
  void operator=(const vtkSMMultiViewFactory&); // Not implemented

  class vtkVector;
  vtkVector* RenderViews;
//ETX
};

#endif

