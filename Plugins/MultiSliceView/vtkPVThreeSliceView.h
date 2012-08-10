/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThreeSliceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVThreeSliceView - CLIENT ONLY view that is used to handle the
// composition of 4 Slice View where 3 of them need a custom representation
// output to only pick their slice.

#ifndef __vtkPVThreeSliceView_h
#define __vtkPVThreeSliceView_h

#include "vtkObject.h"

class vtkCollection;
class vtkSMProxy;
class vtkSMRenderViewProxy;

class vtkPVThreeSliceView : public vtkObject
{
public:
  static vtkPVThreeSliceView* New();
  vtkTypeMacro(vtkPVThreeSliceView, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides empty handlers to simulate the vtkPVView API.
  void Initialize(unsigned int) {};

  // Description:
  // Call StillRender() on the root view.
  void StillRender();

  // Description:
  // Call InteractiveRender() on the root view.
  void InteractiveRender();

  // Description:
  // Initialize the vtkPVThreeSliceView giving set of view proxy to be used
  // to create the views.
  void Initialize(vtkSMRenderViewProxy* topLeft, vtkSMRenderViewProxy *topRight,
                  vtkSMRenderViewProxy *bottomLeft, vtkSMRenderViewProxy *bottomRight);

  // Description:
  // Adds a representation proxy to this view.
  void AddRepresentation(vtkSMProxy*);

  // Description:
  // Removes a representation proxy from this view.
  void RemoveRepresentation(vtkSMProxy*);

  // Description:
  // Removes all added representations from this view.
  // Simply calls RemoveRepresentation() on all added representations
  // one by one.
  void RemoveAllRepresentations();

  // Description:
  // Updates the data pipelines for all visible representations.
  void Update();

  // Description:
  // Get/Set the view time.
  vtkSetMacro(ViewTime, double);
  vtkGetMacro(ViewTime, double);

  // Description:
  // Marks the view dirty i.e. on next Update() it needs update the
  // cloned representation accross all the internal 2D slice views.
  void MarkOutdated() { this->Outdated=true; }

  // Description:
  // Returns the view proxy.
  vtkGetObjectMacro(TopLeftView, vtkSMRenderViewProxy);
  vtkGetObjectMacro(TopRightView, vtkSMRenderViewProxy);
  vtkGetObjectMacro(BottomLeftView, vtkSMRenderViewProxy);
  vtkGetObjectMacro(BottomRightView, vtkSMRenderViewProxy);

  void UpdateAllRepresentations();

  //BTX
protected:
  vtkPVThreeSliceView();
  ~vtkPVThreeSliceView();

  // Description:
  // Get all internal vtkSMRepresentations for a given view.  If the given
  // view is not managed by this ThreeSliceView view it will be ignored.  The
  // representations should only be used by the GUI for creating representation
  // clones.  It is not recommended to directly change the properties of the returned
  // representations.
  void GetRepresentationsForView(vtkSMRenderViewProxy*, vtkCollection*);

  // Clear Representation cache
  void ClearDataCaches();

  double ViewTime;
  bool Outdated;

  void SetTopLeftView(vtkSMRenderViewProxy*);
  void SetTopRightView(vtkSMRenderViewProxy*);
  void SetBottomLeftView(vtkSMRenderViewProxy*);
  void SetBottomRightView(vtkSMRenderViewProxy*);

  vtkSMRenderViewProxy* TopLeftView;
  vtkSMRenderViewProxy* TopRightView;
  vtkSMRenderViewProxy* BottomLeftView;
  vtkSMRenderViewProxy* BottomRightView;

private:
  vtkPVThreeSliceView(const vtkPVThreeSliceView&); // Not implemented
  void operator=(const vtkPVThreeSliceView&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
  vtkCommand* MarkOutdatedObserver;

  //ETX
};

#endif
