/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeView - view for comparative visualization/
// film-strips.
// .SECTION Description
// vtkPVComparativeView is the view used to generate/view comparative
// visualizations/film-strips. This is not a proxy

#ifndef __vtkPVComparativeView_h
#define __vtkPVComparativeView_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkObject.h"

class vtkCollection;
class vtkSMComparativeAnimationCueProxy;
class vtkSMProxy;
class vtkSMViewProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkPVComparativeView : public vtkObject
{
public:
  static vtkPVComparativeView* New();
  vtkTypeMacro(vtkPVComparativeView, vtkObject);
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
  // Initialize the vtkPVComparativeView giving the root view proxy to be used
  // to create the comparative views.
  void Initialize(vtkSMViewProxy* rootView);

  // Description:
  // Builds the MxN views. This method simply creates the MxN internal view modules.
  // It does not generate the visualization i.e. play the animation scene(s).
  // This method does nothing unless the dimensions have changed, in which case
  // it creates new internal view modules (or destroys extra ones). Note that
  // the it's the responsibility of the application to lay the views out so that
  // they form a MxN grid.
  void Build(int dx, int dy);

  // Description:
  // When set to true, all comparisons are shown in the same view. Otherwise,
  // they are tiled in separate views.
  void SetOverlayAllComparisons(bool);
  vtkGetMacro(OverlayAllComparisons, bool);

  // Description:
  // Returns the dimensions used by the most recent Build() request.
  vtkGetVector2Macro(Dimensions, int);

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
  vtkGetObjectMacro(RootView, vtkSMViewProxy);

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  void SetViewSize(int x, int y)
    {
    this->ViewSize[0] = x;
    this->ViewSize[1] = y;
    this->UpdateViewLayout();
    }

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  void SetViewPosition(int x, int y)
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
  // Get/Set the view time.
  vtkGetMacro(ViewTime, double);
  void SetViewTime (double time)
  {
    if (this->ViewTime != time)
      {
      this->ViewTime = time;
      this->Modified();
      this->MarkOutdated();
      }
  }

  // Description:
  // Marks the view dirty i.e. on next Update() it needs to regenerate the
  // comparative vis by replaying the animation(s).
  void MarkOutdated()
    { this->Outdated=true; }

  //BTX
protected:
  vtkPVComparativeView();
  ~vtkPVComparativeView();

  // Description:
  // Creates and appends a new internal view.
  // This not only creates a new view but also new copies of representations
  // for all the representations in the view and adds them to the new view.
  void AddNewView();

  // Description:
  // Removes an internal view and all the representations in that view.
  void RemoveView(vtkSMViewProxy* remove);

  // Description:
  // Update layout for internal views.
  void UpdateViewLayout();

  // Description:
  // Update all representations belonging for the indicated position.
  void UpdateAllRepresentations(int x, int y);

  // Description:
  // Clears the cached data for representations belonging to the indicated
  // position.
  void ClearDataCaches(int x, int y);

  int Dimensions[2];
  int ViewSize[2];
  int ViewPosition[2];
  int Spacing[2];
  double ViewTime;
  bool OverlayAllComparisons;
  bool Outdated;

  void SetRootView(vtkSMViewProxy*);
  vtkSMViewProxy* RootView;
private:
  vtkPVComparativeView(const vtkPVComparativeView&); // Not implemented
  void operator=(const vtkPVComparativeView&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
  vtkCommand* MarkOutdatedObserver;

  //ETX
};

#endif
