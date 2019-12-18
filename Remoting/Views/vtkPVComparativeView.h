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
/**
 * @class   vtkPVComparativeView
 * @brief   view for comparative visualization/
 * film-strips.
 *
 * vtkPVComparativeView is the view used to generate/view comparative
 * visualizations/film-strips. This is not a proxy
*/

#ifndef vtkPVComparativeView_h
#define vtkPVComparativeView_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkCollection;
class vtkImageData;
class vtkSMComparativeAnimationCueProxy;
class vtkSMProxy;
class vtkSMViewProxy;

class VTKREMOTINGVIEWS_EXPORT vtkPVComparativeView : public vtkObject
{
public:
  static vtkPVComparativeView* New();
  vtkTypeMacro(vtkPVComparativeView, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides empty handlers to simulate the vtkPVView API.
   */
  void Initialize(unsigned int){};

  /**
   * Call StillRender() on the root view.
   */
  void StillRender();

  /**
   * Call InteractiveRender() on the root view.
   */
  void InteractiveRender();

  /**
   * Initialize the vtkPVComparativeView giving the root view proxy to be used
   * to create the comparative views.
   */
  void Initialize(vtkSMViewProxy* rootView);

  /**
   * Builds the MxN views. This method simply creates the MxN internal view modules.
   * It does not generate the visualization i.e. play the animation scene(s).
   * This method does nothing unless the dimensions have changed, in which case
   * it creates new internal view modules (or destroys extra ones). Note that
   * the it's the responsibility of the application to lay the views out so that
   * they form a MxN grid.
   */
  void Build(int dx, int dy);

  //@{
  /**
   * When set to true, all comparisons are shown in the same view. Otherwise,
   * they are tiled in separate views.
   */
  void SetOverlayAllComparisons(bool);
  vtkGetMacro(OverlayAllComparisons, bool);
  //@}

  //@{
  /**
   * Returns the dimensions used by the most recent Build() request.
   */
  vtkGetVector2Macro(Dimensions, int);
  //@}

  /**
   * Adds a representation proxy to this view.
   */
  void AddRepresentation(vtkSMProxy*);

  /**
   * Removes a representation proxy from this view.
   */
  void RemoveRepresentation(vtkSMProxy*);

  /**
   * Updates the data pipelines for all visible representations.
   */
  void Update();

  /**
   * Get all the internal views. The views should only be used to be laid out
   * by the GUI. It's not recommended to directly change the properties of the
   * views.
   */
  void GetViews(vtkCollection* collection);

  //@{
  /**
   * Returns the root view proxy.
   */
  vtkGetObjectMacro(RootView, vtkSMViewProxy);
  //@}

  //@{
  /**
   * ViewSize, ViewPosition need to split up among all the component
   * views correctly.
   */
  void SetViewSize(int x, int y)
  {
    this->ViewSize[0] = x;
    this->ViewSize[1] = y;
    this->UpdateViewLayout();
  }
  //@}

  //@{
  /**
   * ViewSize, ViewPosition need to split up among all the component
   * views correctly.
   */
  void SetViewPosition(int x, int y)
  {
    this->ViewPosition[0] = x;
    this->ViewPosition[1] = y;
    this->UpdateViewLayout();
  }
  //@}

  //@{
  /**
   * When saving screenshots with tiling, these methods get called.
   * Not to be confused with tile scale and viewport setup on tile display.
   *
   * @sa vtkViewLayout::UpdateLayoutForTileDisplay
   */
  void SetTileScale(int x, int y);
  void SetTileViewport(double x0, double y0, double x1, double y1);
  //@}

  /**
   * Satisfying vtkPVView API. We don't need to do anything here since the
   * subviews have their own PPI settings.
   */
  void SetPPI(int) {}

  //@{
  /**
   * Set spacing between views.
   */
  vtkSetVector2Macro(Spacing, int);
  vtkGetVector2Macro(Spacing, int);
  //@}

  //@{
  /**
   * Add/Remove parameter cues.
   */
  void AddCue(vtkSMComparativeAnimationCueProxy*);
  void RemoveCue(vtkSMComparativeAnimationCueProxy*);
  //@}

  //@{
  /**
   * Get/Set the view time.
   */
  vtkGetMacro(ViewTime, double);
  void SetViewTime(double time)
  {
    if (this->ViewTime != time)
    {
      this->ViewTime = time;
      this->Modified();
      this->MarkOutdated();
    }
  }
  //@}

  /**
   * Marks the view dirty i.e. on next Update() it needs to regenerate the
   * comparative vis by replaying the animation(s).
   */
  void MarkOutdated() { this->Outdated = true; }

  /**
   * These methods mimic the vtkPVView API. They do nothing here since each view
   * internal view will call PrepareForScreenshot and CleanupAfterScreenshot
   * explicitly when we capture the images from each of them as needed.
   */
  void PrepareForScreenshot() {}
  void CleanupAfterScreenshot() {}
  vtkImageData* CaptureWindow(int magX, int magY);

protected:
  vtkPVComparativeView();
  ~vtkPVComparativeView() override;

  /**
   * Update layout for internal views.
   */
  void UpdateViewLayout();

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
  vtkPVComparativeView(const vtkPVComparativeView&) = delete;
  void operator=(const vtkPVComparativeView&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
  vtkCommand* MarkOutdatedObserver;
};

#endif
