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

class vtkPVQuadRenderView : public vtkPVMultiSliceView
{
public:
  static vtkPVQuadRenderView* New();
  vtkTypeMacro(vtkPVQuadRenderView, vtkPVMultiSliceView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  virtual void Initialize(unsigned int id);

  // Description:
  // Set the position on this view in the multiview configuration.
  // This can be called only after Initialize().
  virtual void SetPosition(int, int);

  enum ViewTypes
    {
    TOP_LEFT = 0,
    TOP_RIGHT = 1,
    BOTTOM_LEFT = 2
    };

  vtkRenderWindow* GetOrthoViewWindow(ViewTypes type);
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

  vtkPVRenderView* GetOrthoRenderView(ViewTypes type);
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

  void SetSizeTopLeft(int x, int y) { this->SetOrthoSize(TOP_LEFT, x, y); }
  void SetSizeBottomLeft(int x, int y) { this->SetOrthoSize(BOTTOM_LEFT, x, y); }
  void SetSizeTopRight(int x, int y) { this->SetOrthoSize(TOP_RIGHT, x, y); }
  void SetOrthoSize(ViewTypes type, int x, int y);

  // Description:
  // Set the bottom-right window size, which is same this superclass' size.
  void SetSizeBottomRight(int x, int y) { this->Superclass::SetSize(x, y); }

  virtual void ResetCamera();
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

  void SetViewUpTopLeft(double x, double y, double z);
  void SetViewUpTopRight(double x, double y, double z);
  void SetViewUpBottomLeft(double x, double y, double z);

//BTX
protected:
  vtkPVQuadRenderView();
  ~vtkPVQuadRenderView();

  virtual void Render(bool interactive, bool skip_rendering);

  struct OrthoViewInfo
    {
    vtkSmartPointer<vtkPVRenderView> RenderView;
    };

  OrthoViewInfo OrthoViews[3];

private:
  vtkPVQuadRenderView(const vtkPVQuadRenderView&); // Not implemented
  void operator=(const vtkPVQuadRenderView&); // Not implemented
//ETX
};

#endif
