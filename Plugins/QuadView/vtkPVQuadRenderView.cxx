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
#include "vtkPVQuadRenderView.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractWidget.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPointSource.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkWeakPointer.h"

#include <sstream>
#include <set>

class vtkQuadInternal;
vtkQuadInternal* QuadInternal;
//============================================================================
class vtkPVQuadRenderView::vtkQuadInternal
{
public:
  vtkQuadInternal(vtkPVQuadRenderView* parent)
  {
    this->Owner = parent;
    this->ObserverId = 0;
    double defaultTX[5] = {1,0,0,1,0};
    double defaultTY[5] = {0,1,0,1,0};
    double defaultTZ[5] = {0,0,1,1,0};
    this->Copy(defaultTX, this->TranformX, 5);
    this->Copy(defaultTY, this->TranformY, 5);
    this->Copy(defaultTZ, this->TranformZ, 5);

    this->Owner->GetOrthoRenderView(TOP_LEFT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesX.GetPointer());
    this->Owner->GetOrthoRenderView(TOP_RIGHT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesY.GetPointer());
    this->Owner->GetOrthoRenderView(BOTTOM_LEFT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesZ.GetPointer());
//    this->CoordinatesX->SetPosition(.95,.05);
//    this->CoordinatesX->GetTextProperty()->SetJustificationToRight();
//    this->CoordinatesX->GetTextProperty()->SetVerticalJustificationToBottom();
//    this->CoordinatesY->SetPosition(.05,.05);
//    this->CoordinatesY->GetTextProperty()->SetJustificationToLeft();
//    this->CoordinatesY->GetTextProperty()->SetVerticalJustificationToBottom();
//    this->CoordinatesZ->SetPosition(.95,.95);
//    this->CoordinatesZ->GetTextProperty()->SetJustificationToRight();
//    this->CoordinatesZ->GetTextProperty()->SetVerticalJustificationToTop();
    for(int i=0; i < 3; ++i)
      {
      this->NaturalCoordinates[i] = 0.0;
      this->TextValues[i] = 0.0;
      }
    // Scalar init
    this->TextValues[3] = 0.0;
  }

  void SetSliceOriginSource(vtkPointSource* source)
  {
    if(this->ObserverId && this->SliceOriginSource)
      {
      this->SliceOriginSource->RemoveObserver(this->ObserverId);
      this->ObserverId = 0;
      }

    this->SliceOriginSource = source;
    if(this->SliceOriginSource)
      {
      this->ObserverId =
          this->SliceOriginSource->AddObserver(
            vtkCommand::AnyEvent, this->Owner,
            &vtkPVQuadRenderView::WidgetCallback);
      }
  }

  void UpdateSliceOrigin(double* origin)
  {
    for(int i=0; i < 3; ++i)
      {
      this->TextValues[i] = origin[i];
      }
    this->UpdateLabels();
  }

  void SetScalarValue(double value)
  {
    this->TextValues[3] = value;
    this->UpdateLabels();
  }

  double GetScalarValue()
  {
    return this->TextValues[3];
  }

  void UpdateLabels()
  {
    this->ComputeNaturalCoordinates();
    std::stringstream zy,xy,zx;
    if(this->Owner->GetXAxisLabel())
      {
      xy << this->Owner->GetXAxisLabel() << "=";
      }
    xy << this->NaturalCoordinates[0] << ", ";
    if(this->Owner->GetYAxisLabel())
      {
      xy << this->Owner->GetYAxisLabel() << "=";
      }
    xy << this->NaturalCoordinates[1]; // Done
    if(this->Owner->GetZAxisLabel())
      {
      zy << this->Owner->GetZAxisLabel() << "=";
      zx << this->Owner->GetZAxisLabel() << "=";
      }
    zy << this->NaturalCoordinates[2] << ", ";
    zx << this->NaturalCoordinates[2] << ", ";
    if(this->Owner->GetXAxisLabel())
      {
      zx << this->Owner->GetXAxisLabel() << "=";
      }
    zx << this->NaturalCoordinates[0]; // Done
    if(this->Owner->GetYAxisLabel())
      {
      zy << this->Owner->GetYAxisLabel() << "=";
      }
    zy << this->NaturalCoordinates[1]; // Done

    // Scalar
    if(this->Owner->GetScalarLabel())
      {
      xy << ", " << this->Owner->GetScalarLabel() << "=" << this->TextValues[3];
      zy << ", " << this->Owner->GetScalarLabel() << "=" << this->TextValues[3];
      zx << ", " << this->Owner->GetScalarLabel() << "=" << this->TextValues[3];
      }

    this->CoordinatesX->SetInput(zy.str().c_str());
    this->CoordinatesY->SetInput(zx.str().c_str());
    this->CoordinatesZ->SetInput(xy.str().c_str());
  }

  void UpdateHandleSize()
  {
    int viewSize[2];
    this->Owner->GetSize(viewSize);
    int size = 2*std::max(viewSize[0], viewSize[1]);

    std::set<vtkWeakPointer<vtkPointHandleRepresentation3D> >::iterator iter;
    for(iter = this->PointHandles.begin(); iter != this->PointHandles.end(); iter++)
      {
      if(*iter)
        {
        iter->GetPointer()->SetHandleSize(size);
        }
      }
  }

  void AddWidget(vtkDataRepresentation* rep)
  {
    vtk3DWidgetRepresentation* widget = vtk3DWidgetRepresentation::SafeDownCast(rep);
    if(widget)
      {
      vtkPointHandleRepresentation3D* pointRep = vtkPointHandleRepresentation3D::SafeDownCast(widget->GetRepresentation());
      if(pointRep)
        {
        this->PointHandles.insert(pointRep);
        }
      }
  }

  void RemoveWidget(vtkDataRepresentation* rep)
  {
    vtk3DWidgetRepresentation* widget = vtk3DWidgetRepresentation::SafeDownCast(rep);
    if(widget)
      {
      vtkPointHandleRepresentation3D* pointRep = vtkPointHandleRepresentation3D::SafeDownCast(widget->GetRepresentation());
      if(pointRep)
        {
        this->PointHandles.erase(pointRep);
        }
      }
  }

  void ComputeNaturalCoordinates()
  {
    this->NaturalCoordinates[0] = ((this->TextValues[0] / this->TranformX[0]) - (this->TextValues[1] * this->TranformY[0]) -  (this->TextValues[2] * this->TranformZ[0])) * this->TranformX[3] + this->TranformX[4];
    this->NaturalCoordinates[1] = ((this->TextValues[1] / this->TranformY[1]) - (this->TextValues[0] * this->TranformX[1]) -  (this->TextValues[2] * this->TranformZ[1])) * this->TranformY[3] + this->TranformY[4];
    this->NaturalCoordinates[2] = ((this->TextValues[2] / this->TranformZ[2]) - (this->TextValues[0] * this->TranformX[2]) -  (this->TextValues[1] * this->TranformY[2])) * this->TranformZ[3] + this->TranformZ[4];
  }

  void UpdateLabelFontSize(int fontSize)
  {
    this->CoordinatesX->GetTextProperty()->SetFontSize(fontSize);
    this->CoordinatesY->GetTextProperty()->SetFontSize(fontSize);
    this->CoordinatesZ->GetTextProperty()->SetFontSize(fontSize);
  }

  void UpdateTransformX(double coef[5])
  {
    this->Copy(coef, this->TranformX, 5);
  }
  void UpdateTransformY(double coef[5])
  {
    this->Copy(coef, this->TranformY, 5);
  }
  void UpdateTransformZ(double coef[5])
  {
    this->Copy(coef, this->TranformZ, 5);
  }

  void Copy(double* a, double* b, int size)
  {
    for(int i=0;i<size;i++)
      {
      b[i] = a[i];
      }
  }

private:
  vtkPVQuadRenderView* Owner;
  unsigned long ObserverId;
  vtkWeakPointer<vtkPointSource> SliceOriginSource;
  double TextValues[4];
  double NaturalCoordinates[3];
  vtkNew<vtkTextActor> CoordinatesX;
  vtkNew<vtkTextActor> CoordinatesY;
  vtkNew<vtkTextActor> CoordinatesZ;
  std::set<vtkWeakPointer<vtkPointHandleRepresentation3D> > PointHandles;
  double TranformX[5];
  double TranformY[5];
  double TranformZ[5];
};

//============================================================================
vtkStandardNewMacro(vtkPVQuadRenderView);
//----------------------------------------------------------------------------
vtkPVQuadRenderView::vtkPVQuadRenderView()
{
  this->OrientationAxesVisibility = true;
  this->SliceOrientationAxesVisibility = 0;
  this->ShowCubeAxes = 0;
  this->ShowOutline = 1;
  this->SplitRatio[0] = this->SplitRatio[1] = .5;
  this->LabelFontSize = 20;
  this->ViewPosition[0] = this->ViewPosition[1] = 0;
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView = vtkSmartPointer<vtkPVRenderView>::New();
    this->OrthoViews[cc].RenderView->GetRenderer()->GetActiveCamera()->ParallelProjectionOn();
    this->OrthoViews[cc].RenderView->SetInteractionMode(INTERACTION_MODE_2D);
    this->OrthoViews[cc].RenderView->SetCenterAxesVisibility(false);
    }

  // Create a single slice for each axis by default
  for(int i=0; i < 3; ++i)
    {
    this->SetNumberOfSlice(i, 1);
    this->SetSlice(i, 0, 0);
    }

  this->XAxisLabel = this->YAxisLabel = this->ZAxisLabel = this->ScalarLabel = NULL;
  this->QuadInternal = new vtkQuadInternal(this);
  this->QuadInternal->UpdateLabelFontSize(this->LabelFontSize);
}

//----------------------------------------------------------------------------
vtkPVQuadRenderView::~vtkPVQuadRenderView()
{
  delete this->QuadInternal;
  this->QuadInternal = NULL;
  this->SetXAxisLabel(NULL);
  this->SetYAxisLabel(NULL);
  this->SetZAxisLabel(NULL);
  this->SetScalarLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
    {
    // already initialized
    return;
    }
  this->Superclass::Initialize(id);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->Initialize(id+cc+1);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewPosition(int posx, int posy)
{
  this->ViewPosition[0] = posx;
  this->ViewPosition[1] = posy;
  if (this->Identifier == 0)
    {
    this->Superclass::SetPosition(posx, posy);
    return;
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVQuadRenderView::GetOrthoViewWindow(
  vtkPVQuadRenderView::ViewTypes type)
{
  return this->OrthoViews[type].RenderView->GetRenderWindow();
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVQuadRenderView::GetOrthoRenderView(ViewTypes type)
{
  return this->OrthoViews[type].RenderView;
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetOrthoSize(ViewTypes type, int x, int y)
{
  this->OrthoViews[type].RenderView->SetSize(x, y);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Render(bool interactive, bool skip_rendering)
{
  this->Superclass::Render(interactive, skip_rendering);

  for (int cc=0; (cc < 3) && !skip_rendering && !this->GetMakingSelection(); cc++)
    {
    if (interactive)
      {
      this->OrthoViews[cc].RenderView->InteractiveRender();
      }
    else
      {
      this->OrthoViews[cc].RenderView->StillRender();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::ResetCamera()
{
  this->Superclass::ResetCamera();
  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->ResetCamera(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::ResetCamera(double bounds[6])
{
  this->Superclass::ResetCamera(bounds);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->ResetCamera(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalTopLeft(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(TOP_LEFT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3];
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice X
  this->SetSliceXNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalTopRight(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(TOP_RIGHT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3] = {0,0,0};
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice Y
  this->SetSliceYNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalBottomLeft(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(BOTTOM_LEFT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3] = {0,0,0};
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice Z
  this->SetSliceZNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpTopLeft(double x, double y, double z)
{
  this->GetOrthoRenderView(TOP_LEFT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpTopRight(double x, double y, double z)
{
  this->GetOrthoRenderView(TOP_RIGHT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpBottomLeft(double x, double y, double z)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetCamera3DManipulators(const int types[9])
{
  this->Superclass::SetCamera3DManipulators(types);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetCamera3DManipulators(types);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetCamera2DManipulators(const int types[9])
{
  this->Superclass::SetCamera2DManipulators(types);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetCamera2DManipulators(types);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackground(double r, double g, double b)
{
  this->Superclass::SetBackground(r,g,b);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackground(r,g,b);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackground2(double r, double g, double b)
{
  this->Superclass::SetBackground2(r,g,b);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackground2(r,g,b);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackgroundTexture(vtkTexture* val)
{
  this->Superclass::SetBackgroundTexture(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackgroundTexture(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetGradientBackground(int val)
{
  this->Superclass::SetGradientBackground(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetGradientBackground(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetTexturedBackground(int val)
{
  this->Superclass::SetTexturedBackground(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetTexturedBackground(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetOrientationAxesVisibility(bool v)
{
  this->OrientationAxesVisibility = v;
  this->Superclass::SetOrientationAxesVisibility(v);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetOrientationAxesVisibility(
          v && (this->SliceOrientationAxesVisibility != 0));
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetSliceOrientationAxesVisibility(int v)
{
  this->SliceOrientationAxesVisibility = v;
  this->SetOrientationAxesVisibility(this->OrientationAxesVisibility);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetOrientationAxesInteractivity(bool v)
{
  this->Superclass::SetOrientationAxesInteractivity(v);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetOrientationAxesInteractivity(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToTopLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_LEFT)->AddRepresentation(rep);
  this->QuadInternal->AddWidget(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToTopRight(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_RIGHT)->AddRepresentation(rep);
  this->QuadInternal->AddWidget(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToBottomLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->AddRepresentation(rep);
  this->QuadInternal->AddWidget(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToTopLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_LEFT)->RemoveRepresentation(rep);
  this->QuadInternal->RemoveWidget(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToTopRight(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_RIGHT)->RemoveRepresentation(rep);
  this->QuadInternal->RemoveWidget(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToBottomLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->RemoveRepresentation(rep);
  this->QuadInternal->RemoveWidget(rep);
}
//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetSliceOriginSource(vtkPointSource* source)
{
  this->QuadInternal->SetSliceOriginSource(source);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::WidgetCallback(vtkObject* src, unsigned long, void*)
{
  vtkPointSource* source = vtkPointSource::SafeDownCast(src);
  if(source)
    {
    double* center = source->GetCenter();
    for(int i=0;i<3;++i)
      {
      this->SetSliceOrigin(i, center[0], center[1], center[2]);
      }
    this->QuadInternal->UpdateSliceOrigin(center);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Update()
{
  this->UpdateViewLayout();
  this->Superclass::Update();
  for(int i=0; i < 3; ++i)
    {
    this->GetOrthoRenderView(i)->CopyViewUpdateOptions(this);
    }
  this->QuadInternal->UpdateHandleSize();
}
//----------------------------------------------------------------------------
void vtkPVQuadRenderView::UpdateViewLayout()
{
  const int spacing = 2;
  int posx = this->ViewPosition[0];
  int posy = this->ViewPosition[1];

  int size[2];
  this->OrthoViews[TOP_LEFT].RenderView->GetSize(size);

  this->OrthoViews[TOP_LEFT].RenderView->SetPosition(posx, posy);
  this->OrthoViews[BOTTOM_LEFT].RenderView->SetPosition(posx, posy + size[1] + spacing);
  this->OrthoViews[TOP_RIGHT].RenderView->SetPosition(posx + size[0] + spacing, posy);
  this->Superclass::SetPosition(posx + size[0] + spacing, posy + size[1] + spacing);
}
//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetScalarValue(double value)
{
  this->QuadInternal->SetScalarValue(value);
}

//----------------------------------------------------------------------------
double vtkPVQuadRenderView::GetScalarValue()
{
  return this->QuadInternal->GetScalarValue();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetTransformationForX(double coef[5])
{
  this->QuadInternal->UpdateTransformX(coef);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetTransformationForY(double coef[5])
{
  this->QuadInternal->UpdateTransformY(coef);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetTransformationForZ(double coef[5])
{
  this->QuadInternal->UpdateTransformZ(coef);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetLabelFontSize(int value)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting LabelFontSize to " << value);
  if (this->LabelFontSize != value)
    {
    this->LabelFontSize = value;
    this->QuadInternal->UpdateLabelFontSize(this->LabelFontSize);
    this->Modified();
    }
}
