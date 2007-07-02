/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMComparativeViewProxy - view for comparative visualization/
// time-strips.
// .SECTION Description
// vtkSMComparativeViewProxy is the view used to generate/view comparative
// visualizations/time strips.

#ifndef __vtkSMComparativeViewProxy_h
#define __vtkSMComparativeViewProxy_h

#include "vtkSMViewProxy.h"

class VTK_EXPORT vtkSMComparativeViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMComparativeViewProxy* New();
  vtkTypeRevisionMacro(vtkSMComparativeViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Builds the MxN views. This method simply creates the MxN internal view modules.
  // It does not generate the visualization i.e. play the animation scene(s).
  // This method does nothing unless the dimensions have changed, in which case
  // it creates new internal view modules (or destroys extra ones). Note that
  // the it's the responsibility of the application to lay the views out so that
  // they form a MxN grid. 
  void Build(int dx, int dy);

  // Description:
  // Returns the dimensions used by the most recent Build() request.
  vtkGetVector2Macro(Dimensions, int);

  // Description:
  // Adds a representation proxy to this view. 
  virtual void AddRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes a representation proxy from this view.
  virtual void RemoveRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes all added representations from this view.
  // Simply calls RemoveRepresentation() on all added representations 
  // one by one.
  void RemoveAllRepresentations();

  // Description:
  // Forwards the call to all internal views.
  void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  // Forwards the call to all internal views.
  void InteractiveRender();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  // Overridden to forward the call to the internal root view proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*);

//BTX
protected:
  vtkSMComparativeViewProxy();
  ~vtkSMComparativeViewProxy();

  // Description:
  // Called at start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Creates and appends a new internal view.
  // This not only creates a new view but also new copies of representations
  // for all the representations in the view and adds them to the new view.
  void AddNewView();

  // Description:
  // Removes an internal view and all the representations in that view.
  void RemoveView(vtkSMViewProxy* remove);


  // Description:
  // Returns the root view proxy.
  vtkSMViewProxy* GetRootView();

  int Dimensions[2];

private:
  vtkSMComparativeViewProxy(const vtkSMComparativeViewProxy&); // Not implemented
  void operator=(const vtkSMComparativeViewProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;

//ETX
};

#endif

