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
// film-strips.
// .SECTION Description
// vtkSMComparativeViewProxy is the view used to generate/view comparative
// visualizations/film-strips.

#ifndef __vtkSMComparativeViewProxy_h
#define __vtkSMComparativeViewProxy_h

#include "vtkSMViewProxy.h"

class vtkSMComparativeAnimationCueProxy;
class vtkCollection;

class VTK_EXPORT vtkSMComparativeViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMComparativeViewProxy* New();
  vtkTypeMacro(vtkSMComparativeViewProxy, vtkSMViewProxy);
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
  void SetOverlayAllComparisons(bool);
  vtkGetMacro(OverlayAllComparisons, bool);

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
  virtual void RemoveAllRepresentations();

  // Description:
  // Updates the data pipelines for all visible representations.
  virtual void Update();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  // Overridden to forward the call to the internal root view proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  // Description:
  // Get all the internal views. The views should only be used to be layed out
  // by the GUI. It's not recommended to directly change the properties of the
  // views.
  void GetViews(vtkCollection* collection);

  // Description:
  // Get all internal vtkSMRepresentations for a given view.  If the given
  // view is not managed by this comparative view it will be ignored.  The
  // representations should only be used by the GUI for creating representation
  // clones.  It is not recommended to directly change the properties of the returned
  // representations.
  void GetRepresentationsForView(vtkSMViewProxy*, vtkCollection*);
  void GetRepresentations(int x, int y, vtkCollection*);

  // Description:
  // Returns the root view proxy.
  vtkSMViewProxy* GetRootView();

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  virtual void SetViewSize(int x, int y)
    {
    this->ViewSize[0] = x;
    this->ViewSize[1] = y;
    this->UpdateViewLayout();
    }

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  virtual void SetViewPosition(int x, int y)
    {
    this->ViewPosition[0] = x;
    this->ViewPosition[1] = y;
    this->UpdateViewLayout();
    }

  // Description:
  // Set spacing between views.
  vtkSetVector2Macro(Spacing, int);
  vtkGetVector2Macro(Spacing, int);

  // Description:
  // Add/Remove parameter cues.
  void AddCue(vtkSMComparativeAnimationCueProxy*);
  void RemoveCue(vtkSMComparativeAnimationCueProxy*);

  // Description:
  // Dirty means this algorithm will execute during next update.
  // This all marks all consumers as dirty.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Get/Set the view time.
  vtkSetMacro(ViewTime, double);
  vtkGetMacro(ViewTime, double);

  //BTX
protected:
  vtkSMComparativeViewProxy();
  ~vtkSMComparativeViewProxy();

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Creates and appends a new internal view.
  // This not only creates a new view but also new copies of representations
  // for all the representations in the view and adds them to the new view.
  void AddNewView();

  // Description:
  // Removes an internal view and all the representations in that view.
  void RemoveView(vtkSMViewProxy* remove);

  // Description:
  // Passes on the render request to the root view.
  virtual void PostRender(bool interactive);

  // Description:
  // Update layout for internal views.
  void UpdateViewLayout();

  // Description:
  // Update all representations belonging for the indicated position.
  void UpdateAllRepresentations(int x, int y);

  // Description:
  // Marks the view dirty i.e. on next StillRender it needs to regenerate the
  // comparative vis by replaying the animation(s).
  void MarkOutdated()
    { this->Outdated=true; }

  void ClearDataCaches();

  int Dimensions[2];
  int ViewSize[2];
  int ViewPosition[2];
  int Spacing[2];
  double ViewTime;

  bool OverlayAllComparisons;

  bool Outdated;

private:
  vtkSMComparativeViewProxy(const vtkSMComparativeViewProxy&); // Not implemented
  void operator=(const vtkSMComparativeViewProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
  vtkCommand* MarkOutdatedObserver;

  //ETX
};

#endif

