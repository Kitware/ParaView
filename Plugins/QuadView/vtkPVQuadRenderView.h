/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVQuadRenderView - RenderView extension for Quad view.
// .SECTION Description
// vtkPVQuadRenderView extends vtkPVRenderView to add support for 4 views.

#ifndef __vtkPVQuadRenderView_h
#define __vtkPVQuadRenderView_h

#include "vtkPVMultiSliceView.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class vtkPointSource;

class vtkPVQuadRenderView : public vtkPVMultiSliceView
{
public:
  // ViewType enum
  enum ViewTypes
    {
    TOP_LEFT = 0,
    TOP_RIGHT = 1,
    BOTTOM_LEFT = 2
    };

  static vtkPVQuadRenderView* New();
  vtkTypeMacro(vtkPVQuadRenderView, vtkPVMultiSliceView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  virtual void Initialize(unsigned int id);

  // Description:
  // Set the position of the global view
  virtual void SetViewPosition(int, int);

  // Decription:
  // Return the internal vtkRenderWindow corresponding to the given view type.
  vtkRenderWindow* GetOrthoViewWindow(ViewTypes type);

  // Description:
  // Return the internal vtkRenderWindow corresponding to the given view type
  // if valid otherwise return NULL.
  vtkRenderWindow* GetOrthoViewWindow(int type)
    {
    switch (type)
      {
    case TOP_RIGHT:
    case TOP_LEFT:
    case BOTTOM_LEFT:
      return this->GetOrthoViewWindow(static_cast<ViewTypes>(type));
      }
    return NULL;
    }

  // Decription:
  // Return the internal vtkPVRenderView corresponding to the given view type.
  vtkPVRenderView* GetOrthoRenderView(ViewTypes type);

  // Description:
  // Return the internal vtkPVRenderView corresponding to the given view type
  // if valid otherwise return NULL.
  vtkPVRenderView* GetOrthoRenderView(int type)
    {
    switch (type)
      {
    case TOP_RIGHT:
    case TOP_LEFT:
    case BOTTOM_LEFT:
      return this->GetOrthoRenderView(static_cast<ViewTypes>(type));
      }
    return NULL;
    }

  // Description:
  // Set the size of the internal RenderView for TopLeft
  void SetSizeTopLeft(int x, int y) { this->SetOrthoSize(TOP_LEFT, x, y); }

  // Description:
  // Set the size of the internal RenderView for BottomLeft
  void SetSizeBottomLeft(int x, int y) { this->SetOrthoSize(BOTTOM_LEFT, x, y); }

  // Description:
  // Set the size of the internal RenderView for TopRight
  void SetSizeTopRight(int x, int y) { this->SetOrthoSize(TOP_RIGHT, x, y); }

  // Description:
  // Set the size of the internal RenderView for the provided view type.
  void SetOrthoSize(ViewTypes type, int x, int y);

  // Description:
  // Set the bottom-right window size, which is same this superclass' size.
  void SetSizeBottomRight(int x, int y) { this->Superclass::SetSize(x, y); }

  // Description:
  // Override update so the decision made in the main view are shared across all
  // other internal views.
  virtual void Update();

  // Description:
  // Custom management across all the internal views to avoid the internal
  // Update() call.
  virtual void ResetCamera();

  // Description:
  // Forwarded to all the internal views
  virtual void ResetCamera(double bounds[6]);

  // Description:
  // Set the camera to look perpendicular to the slice as well as setting the
  // slice normal for the "X" ones.
  void SetViewNormalTopLeft(double x, double y, double z);

  // Description:
  // Set the camera to look perpendicular to the slice as well as setting the
  // slice normal for the "Y" ones.
  void SetViewNormalTopRight(double x, double y, double z);

  // Description:
  // Set the camera to look perpendicular to the slice as well as setting the
  // slice normal for the "Z" ones.
  void SetViewNormalBottomLeft(double x, double y, double z);

  // Description:
  // Set the viewUp of the TopLeft view
  void SetViewUpTopLeft(double x, double y, double z);

  // Description:
  // Set the viewUp of the TopLeft view
  void SetViewUpTopRight(double x, double y, double z);

  // Description:
  // Set the viewUp of the TopLeft view
  void SetViewUpBottomLeft(double x, double y, double z);

  // Description:
  // Link this view to an object that is synched accross all process thanks to
  // Proxies. That object hold the origin of all the slices as a single double[3]
  // Internally we attach a listener to it to update the slice origins.
  void SetSliceOriginSource(vtkPointSource* source);

  // Description:
  // Set the position of the split in percent along width and height. (0 < r < 1)
  vtkSetVector2Macro(SplitRatio, double);
  vtkGetVector2Macro(SplitRatio, double);

  // Description:
  // Update label font size
  virtual void SetLabelFontSize(int value);
  vtkGetMacro(LabelFontSize, int);

  // Description:
  // Drive QuadRepresentation configuration
  vtkSetMacro(ShowCubeAxes, int);
  vtkSetMacro(ShowOutline, int);
  vtkGetMacro(ShowCubeAxes, int);
  vtkGetMacro(ShowOutline, int);

  // Override SetOrientationAxesVisibility of the 3D view for 2D views
  void SetSliceOrientationAxesVisibility(int);
  vtkGetMacro(SliceOrientationAxesVisibility, int);

  //*****************************************************************
  // Forwarded accross all the views
  virtual void SetCamera3DManipulators(const int types[9]);
  virtual void SetCamera2DManipulators(const int types[9]);
  virtual void SetBackground(double r, double g, double b);
  virtual void SetBackground2(double r, double g, double b);
  virtual void SetBackgroundTexture(vtkTexture* val);
  virtual void SetGradientBackground(int val);
  virtual void SetTexturedBackground(int val);
  virtual void SetOrientationAxesVisibility(bool);
  virtual void SetOrientationAxesInteractivity(bool);

  //*****************************************************************
  // Helper method that is usefull at the proxy layer to register
  // representations into one of the internal view.
  // In our case we add the point widget that way.
  void AddRepresentationToTopLeft(vtkDataRepresentation* rep);
  void AddRepresentationToTopRight(vtkDataRepresentation* rep);
  void AddRepresentationToBottomLeft(vtkDataRepresentation* rep);
  void RemoveRepresentationToTopLeft(vtkDataRepresentation* rep);
  void RemoveRepresentationToTopRight(vtkDataRepresentation* rep);
  void RemoveRepresentationToBottomLeft(vtkDataRepresentation* rep);

  //*****************************************************************
  // Allow user to set custom label for coordinates
  vtkSetStringMacro(XAxisLabel);
  vtkSetStringMacro(YAxisLabel);
  vtkSetStringMacro(ZAxisLabel);
  vtkSetStringMacro(ScalarLabel);
  vtkGetStringMacro(XAxisLabel);
  vtkGetStringMacro(YAxisLabel);
  vtkGetStringMacro(ZAxisLabel);
  vtkGetStringMacro(ScalarLabel);

  void SetScalarValue(double value);
  double GetScalarValue();

  //*****************************************************************
  // Allow user to trnasform coordinate mapping to reflect natural coordinates
  // based on some arbitrary set of base vector.
  // coef = [ vx, vy, vz, a, b ] where (vx, vy, vz) is the base vector used for X
  // so the natural coordinates could be expressed like:
  // a.(vx, vy, vz) + b
  void SetTransformationForX(double coef[5]);
  void SetTransformationForY(double coef[5]);
  void SetTransformationForZ(double coef[5]);

//BTX
protected:
  vtkPVQuadRenderView();
  ~vtkPVQuadRenderView();

  // Internal method to keep track of SliceOrigin change
  void WidgetCallback(vtkObject* src, unsigned long event, void* data);

  // Custom render method to deal with internal views
  virtual void Render(bool interactive, bool skip_rendering);

  // Internal method used to update layout of the views
  virtual void UpdateViewLayout();
  int ViewPosition[2];
  double SplitRatio[2];

  // Label font size
  int LabelFontSize;

  struct OrthoViewInfo { vtkSmartPointer<vtkPVRenderView> RenderView; };
  OrthoViewInfo OrthoViews[3];
  char* XAxisLabel;
  char* YAxisLabel;
  char* ZAxisLabel;
  char* ScalarLabel;

  int ShowOutline;
  int ShowCubeAxes;
  int SliceOrientationAxesVisibility;
  bool OrientationAxesVisibility;
private:
  vtkPVQuadRenderView(const vtkPVQuadRenderView&); // Not implemented
  void operator=(const vtkPVQuadRenderView&); // Not implemented

  class vtkQuadInternal;
  vtkQuadInternal* QuadInternal;
//ETX
};

#endif
