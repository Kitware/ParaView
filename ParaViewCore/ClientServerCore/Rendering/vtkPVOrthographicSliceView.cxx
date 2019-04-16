/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrthographicSliceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOrthographicSliceView.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVChangeOfBasisHelper.h"
#include "vtkPVGridAxes3DActor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTextRepresentation.h"

#include <cassert>
#include <sstream>

class vtkPVOrthographicSliceViewInteractorStyle : public vtkPVInteractorStyle
{
  vtkPVInteractorStyle* PrimaryInteractorStyle;
  vtkPVInteractorStyle* OrthographicInteractorStyle;
  vtkRenderer* PrimaryRenderer;
  vtkWeakPointer<vtkPVOrthographicSliceView> View;
  vtkVector2i ClickPosition;
  int ClickCounter;

public:
  static vtkPVOrthographicSliceViewInteractorStyle* New();
  vtkTypeMacro(vtkPVOrthographicSliceViewInteractorStyle, vtkPVInteractorStyle);

  vtkSetObjectMacro(OrthographicInteractorStyle, vtkPVInteractorStyle);
  vtkSetObjectMacro(PrimaryInteractorStyle, vtkPVInteractorStyle);
  vtkSetObjectMacro(PrimaryRenderer, vtkRenderer);
  vtkSetMacro(View, vtkPVOrthographicSliceView*);

  void OnLeftButtonDown() override
  {
    this->Superclass::OnLeftButtonDown();

    vtkVector2i eventPosition;
    this->Interactor->GetEventPosition(eventPosition.GetData());
    if (std::abs(eventPosition[0] - this->ClickPosition[0]) < 2 &&
      std::abs(eventPosition[1] - this->ClickPosition[1]) < 2)
    {
    }
    else
    {
      // reset ClickPosition/ClickCounter since the mouse position is different
      // from last time.
      this->ClickPosition = eventPosition;
      this->ClickCounter = 0;
    }
  }
  void OnLeftButtonUp() override
  {
    this->Superclass::OnLeftButtonUp();

    vtkVector2i eventPosition;
    this->Interactor->GetEventPosition(eventPosition.GetData());
    if (std::abs(eventPosition[0] - this->ClickPosition[0]) < 2 &&
      std::abs(eventPosition[1] - this->ClickPosition[1]) < 2)
    {
      // ClickCounter == 1: down & up happened at same location once.
      // ClickCounter == 2: down & up happened at same location twice! -- double click!!!
      this->ClickCounter++;
    }
    else
    {
      // reset ClickPosition/ClickCounter since the mouse position is different
      // from last time.
      this->ClickPosition = vtkVector2i(-1, -1);
      this->ClickCounter = 0;
    }

    if (this->ClickCounter == 2) // double-click!
    {
      double worldPos[3];
      this->GetEventWorldPosition(worldPos);
      this->View->MoveSlicePosition(this->CurrentRenderer, worldPos);
      this->ClickCounter = 0;
    }
  }

  // Disable wheel-to-zoom in this view.
  void OnMouseWheelForward() override {}
  void OnMouseWheelBackward() override {}

protected:
  vtkPVOrthographicSliceViewInteractorStyle()
    : PrimaryInteractorStyle(NULL)
    , OrthographicInteractorStyle(NULL)
    , PrimaryRenderer(NULL)
    , ClickCounter(0)
  {
  }
  ~vtkPVOrthographicSliceViewInteractorStyle() override
  {
    this->SetPrimaryInteractorStyle(NULL);
    this->SetOrthographicInteractorStyle(NULL);
    this->SetPrimaryRenderer(NULL);
  }

  vtkCameraManipulator* FindManipulator(int button, int shift, int control) override
  {
    if (this->CurrentRenderer == this->PrimaryRenderer)
    {
      return this->PrimaryInteractorStyle->FindManipulator(button, shift, control);
    }
    return this->OrthographicInteractorStyle->FindManipulator(button, shift, control);
  }

  void GetEventWorldPosition(double position[3])
  {
    assert(this->CurrentRenderer && this->Interactor);
    int eventPosition[3] = { 0, 0, 0 };
    this->Interactor->GetEventPosition(eventPosition);

    // Now convert this eventPosition to world coordinates.
    this->CurrentRenderer->SetDisplayPoint(eventPosition[0], eventPosition[1], eventPosition[2]);
    this->CurrentRenderer->DisplayToWorld();

    double worldPoint[4];
    this->CurrentRenderer->GetWorldPoint(worldPoint);
    position[0] = worldPoint[0] / worldPoint[3];
    position[1] = worldPoint[1] / worldPoint[3];
    position[2] = worldPoint[2] / worldPoint[3];
  }

private:
  vtkPVOrthographicSliceViewInteractorStyle(const vtkPVOrthographicSliceViewInteractorStyle&);
  void operator=(const vtkPVOrthographicSliceViewInteractorStyle&);
};

vtkStandardNewMacro(vtkPVOrthographicSliceViewInteractorStyle);

vtkStandardNewMacro(vtkPVOrthographicSliceView);
//----------------------------------------------------------------------------
vtkPVOrthographicSliceView::vtkPVOrthographicSliceView()
  : Renderers()
  , OrthographicInteractorStyle()
  , SlicePositionAxes2D()
  , SlicePositionAxes3D()
  , SliceAnnotations()
  , SliceAnnotationsVisibility(false)
  , MouseWheelForwardEventId(0)
  , MouseWheelBackwardEventId(0)
  , GridAxes3DActorsNeedShallowCopy(false)
  , GridAxes3DActorObserverId(0)
{
  this->CenterAxes->SetVisibility(0);
  this->SliceIncrements[0] = this->SliceIncrements[1] = this->SliceIncrements[2] = 1.0;
  this->SlicePosition[0] = this->SlicePosition[1] = this->SlicePosition[2] = 0.0;

  vtkRenderWindow* window = this->GetRenderWindow();
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetBackground(0.5, 0.5, 0.5);
    this->Renderers[cc]->GetActiveCamera()->SetParallelProjection(1);
    window->AddRenderer(this->Renderers[cc].GetPointer());

    this->SlicePositionAxes2D[cc]->SetComputeNormals(0);
    this->SlicePositionAxes2D[cc]->SetPickable(1);
    this->SlicePositionAxes2D[cc]->SetUseBounds(1);
    this->SlicePositionAxes2D[cc]->SetScale(10, 10, 10);
    this->Renderers[cc]->AddActor(this->SlicePositionAxes2D[cc].GetPointer());

    this->SliceAnnotations[cc]->SetRenderer(this->Renderers[cc].GetPointer());
    this->SliceAnnotations[cc]->SetText("Slice Annotation");
    this->SliceAnnotations[cc]->BuildRepresentation();
    this->SliceAnnotations[cc]->SetWindowLocation(vtkTextRepresentation::AnyLocation);
    this->SliceAnnotations[cc]->GetTextActor()->SetTextScaleModeToNone();
    this->SliceAnnotations[cc]->GetTextActor()->GetTextProperty()->SetJustificationToLeft();
    this->SliceAnnotations[cc]->SetVisibility(0);
    this->Renderers[cc]->AddActor(this->SliceAnnotations[cc].GetPointer());
  }

  this->SlicePositionAxes3D->SetComputeNormals(0);
  this->SlicePositionAxes3D->SetPickable(0);
  this->SlicePositionAxes3D->SetUseBounds(1);
  this->SlicePositionAxes3D->SetScale(10, 10, 10);
  this->GetRenderer()->AddActor(this->SlicePositionAxes3D.GetPointer());

  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetPosition(1, 0, 0);
  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetViewUp(0, 1, 0);

  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetPosition(0, 1, 0);
  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetViewUp(1, 0, 0);

  this->Renderers[XY_PLANE]->GetActiveCamera()->SetPosition(0, 0, 1);
  this->Renderers[XY_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[XY_PLANE]->GetActiveCamera()->SetViewUp(0, 1, 0);

  if (this->InteractorStyle) //  bother creating interactor styles only if superclass did as well.
  {
    this->OrthographicInteractorStyle->SetPrimaryInteractorStyle(this->ThreeDInteractorStyle);
    this->OrthographicInteractorStyle->SetOrthographicInteractorStyle(this->TwoDInteractorStyle);
    this->OrthographicInteractorStyle->SetPrimaryRenderer(this->GetRenderer());
    this->OrthographicInteractorStyle->SetView(this);
  }

  this->SetSlicePosition(0, 0, 0);

  // setup viewports for each of the renderers.
  double viewport[] = { 0, 0, 1, 1 };
  this->ScaleRendererViewports(viewport);
}

//----------------------------------------------------------------------------
vtkPVOrthographicSliceView::~vtkPVOrthographicSliceView()
{
  if (this->GridAxes3DActor && this->GridAxes3DActorObserverId)
  {
    this->GridAxes3DActor->RemoveObserver(this->GridAxes3DActorObserverId);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->MouseWheelForwardEventId);
    this->Interactor->RemoveObserver(this->MouseWheelBackwardEventId);
    this->MouseWheelForwardEventId = 0;
    this->MouseWheelBackwardEventId = 0;
  }
  this->Superclass::SetupInteractor(iren);
  if (this->Interactor)
  {
    this->MouseWheelForwardEventId =
      this->Interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, this,
        &vtkPVOrthographicSliceView::OnMouseWheelForwardEvent);
    this->MouseWheelBackwardEventId =
      this->Interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, this,
        &vtkPVOrthographicSliceView::OnMouseWheelBackwardEvent);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetInteractionMode(int mode)
{
  this->Superclass::SetInteractionMode(mode);
  if (this->Interactor &&
    this->Interactor->GetInteractorStyle() != this->OrthographicInteractorStyle.GetPointer())
  {
    switch (this->InteractionMode)
    {
      case INTERACTION_MODE_3D:
      case INTERACTION_MODE_2D:
        this->OrthographicInteractorStyle->SetPrimaryInteractorStyle(
          vtkPVInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle()));
        this->Interactor->SetInteractorStyle(this->OrthographicInteractorStyle.GetPointer());
        break;

      default:
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::ResetCamera()
{
  this->Superclass::ResetCamera();
  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->ResetCamera(bounds);
    // do this explicitly to ensure the SlicePositionAxes2D doesn't get clipped.
    this->Renderers[cc]->ResetCameraClippingRange();
  }
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVOrthographicSliceView::ResetCamera(double bounds[6])
{
  this->Superclass::ResetCamera(bounds);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->ResetCamera(bounds);
    // do this explicitly to ensure the SlicePositionAxes2D doesn't get clipped.
    this->Renderers[cc]->ResetCameraClippingRange();
  }
}
//----------------------------------------------------------------------------
vtkRenderer* vtkPVOrthographicSliceView::GetRenderer(int rendererType)
{
  switch (rendererType)
  {
    case SAGITTAL_VIEW_RENDERER:
    case AXIAL_VIEW_RENDERER:
    case CORONAL_VIEW_RENDERER:
    {
      int index = rendererType - vtkPVRenderView::NON_COMPOSITED_RENDERER - 1;
      assert(index >= 0 && index < 3);
      return this->Renderers[index].GetPointer();
    }
    default:
      return this->Superclass::GetRenderer(rendererType);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::ScaleRendererViewports(const double viewport[4])
{
  const double size[2] = { viewport[2] - viewport[0], viewport[3] - viewport[1] };
  const double start[2] = { viewport[0], viewport[1] };
  const double mid[2] = { viewport[0] + size[0] / 2, viewport[1] + size[1] / 2 };
  const double end[2] = { viewport[2], viewport[3] };

  double lowerRight[] = { mid[0], start[1], end[0], mid[1] };
  this->Superclass::ScaleRendererViewports(lowerRight);

  double topRight[] = { mid[0], mid[1], end[0], end[1] };
  this->Renderers[SIDE_VIEW]->SetViewport(topRight);

  double topLeft[] = { start[0], mid[1], mid[0], end[1] };
  this->Renderers[TOP_VIEW]->SetViewport(topLeft);

  double lowerLeft[] = { start[0], start[1], mid[0], mid[1] };
  this->Renderers[FRONT_VIEW]->SetViewport(lowerLeft);

  for (int cc = 0; cc < 3; cc++)
  {
    double vp[4];
    this->Renderers[cc]->GetViewport(vp);
    // BUG: one would think x coordinate should be vp[0]. But there's
    // something funny with vtkTextRepresentation. We x coordinate relative to
    // the renderer's viewport, but y coordinate relative to the window!
    this->SliceAnnotations[cc]->SetPosition(0 + 0.01, vp[1] + 0.01);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetSlicePosition(double x, double y, double z)
{
  this->SetNumberOfXSlices(1);
  this->SetXSlices(&x);
  this->SetNumberOfYSlices(1);
  this->SetYSlices(&y);
  this->SetNumberOfZSlices(1);
  this->SetZSlices(&z);

  this->SlicePositionAxes2D[YZ_PLANE]->SetPosition(x + 0.01, y, z);
  this->SlicePositionAxes2D[ZX_PLANE]->SetPosition(x, y + 0.01, z);
  this->SlicePositionAxes2D[XY_PLANE]->SetPosition(x, y, z + 0.01);
  this->SlicePositionAxes3D->SetPosition(x, y, z);

  this->SlicePosition[0] = x;
  this->SlicePosition[1] = y;
  this->SlicePosition[2] = z;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::UpdateCenterAxes()
{
  this->Superclass::UpdateCenterAxes();
  vtkBoundingBox bbox(this->GeometryBounds);
  double widths[3];
  bbox.GetLengths(widths);
  widths[0] *= 2;
  widths[1] *= 2;
  widths[2] *= 2;
  this->SlicePositionAxes2D[0]->SetScale(widths);
  this->SlicePositionAxes2D[1]->SetScale(widths);
  this->SlicePositionAxes2D[2]->SetScale(widths);
  this->SlicePositionAxes3D->SetScale(widths);

  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  for (int cc = 0; cc < 3; cc++)
  {
    if (this->GridAxes3DActors[cc])
    {
      this->GridAxes3DActors[cc]->SetTransformedBounds(bounds);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::AboutToRenderOnLocalProcess(bool interactive)
{
  if (this->ModelTransformationMatrix->GetMTime() > this->ModelTransformationMatrixUpdateTime)
  {
    this->SlicePositionAxes2D[0]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes2D[1]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes2D[2]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes3D->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->ModelTransformationMatrixUpdateTime.Modified();
  }

  // Update camera directions.
  vtkVector3d axis[3];
  vtkPVChangeOfBasisHelper::GetBasisVectors(
    this->ModelTransformationMatrix.GetPointer(), axis[0], axis[1], axis[2]);
  for (int cc = 0; cc < 3; cc++)
  {
    vtkCamera* camera = this->Renderers[cc]->GetActiveCamera();

    double fp[3];
    camera->GetFocalPoint(fp);

    double focaldistance = camera->GetDistance();
    axis[cc].Normalize();
    camera->SetPosition(fp[0] + axis[cc].GetData()[0] * focaldistance,
      fp[1] + axis[cc].GetData()[1] * focaldistance, fp[2] + axis[cc].GetData()[2] * focaldistance);
  }

  // Setup slice annotations.
  // const char* axis_names[] = {"Sagittal", "Axial", "Coronal" };
  const char* view_names[] = { "Right Side", "Top", "Front" };
  const char* axis_names[] = { "X", "Y", "Z" };
  for (int cc = 0; cc < 3; cc++)
  {
    if (const char* label = this->GetAxisLabel(cc))
    {
      axis_names[cc] = label;
      view_names[cc] = label;
    }
  }
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceAnnotations[cc]->SetVisibility(this->SliceAnnotationsVisibility ? 1 : 0);
    std::ostringstream stream;
    stream << view_names[cc] << " View (" << axis_names[cc] << "=" << this->SlicePosition[cc]
           << ")\n";
    stream << axis_names[(cc + 1) % 3] << "=" << this->SlicePosition[(cc + 1) % 3] << ",";
    stream << axis_names[(cc + 2) % 3] << "=" << this->SlicePosition[(cc + 2) % 3];
    this->SliceAnnotations[cc]->SetText(stream.str().c_str());
  }

  if (this->GridAxes3DActorsNeedShallowCopy)
  {
    // Update our clones of the GridAxes3DActor that sit in the 3 orthographic
    // views.
    this->GridAxes3DActorsNeedShallowCopy = false;
    if (this->GridAxes3DActor)
    {
      for (int cc = 0; cc < 3; cc++)
      {
        assert(this->GridAxes3DActors[cc]);
        this->GridAxes3DActors[cc]->ShallowCopy(this->GridAxes3DActor);
        this->GridAxes3DActors[cc]->SetLabelMask(0xff);
      }
      this->GridAxes3DActors[YZ_PLANE]->SetFaceMask(
        vtkGridAxes3DActor::MAX_YZ | vtkGridAxes3DActor::MIN_YZ);
      this->GridAxes3DActors[ZX_PLANE]->SetFaceMask(
        vtkGridAxes3DActor::MAX_ZX | vtkGridAxes3DActor::MIN_ZX);
      this->GridAxes3DActors[XY_PLANE]->SetFaceMask(
        vtkGridAxes3DActor::MAX_XY | vtkGridAxes3DActor::MIN_XY);
    }
  }

  this->Superclass::AboutToRenderOnLocalProcess(interactive);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::Update()
{
  this->SlicePositionAxes3D->SetUseBounds(0);
  // Since vtkGridAxes3DActor is potentially modified in Update (all of those
  // modifications are passed on to our vtkGridAxes3DActor instances in
  // UpdateCenterAxes(), hence we avoid forcing ShallowCopy on Render since that
  // changes too many things.
  bool prev = this->GridAxes3DActorsNeedShallowCopy;
  this->Superclass::Update();
  this->GridAxes3DActorsNeedShallowCopy = prev;
  this->SlicePositionAxes3D->SetUseBounds(1);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::OnMouseWheelForwardEvent()
{
  vtkRenderer* ren = this->Interactor->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);

  double pos[3];
  this->GetSlicePosition(pos);
  for (int cc = 0; cc < 3; cc++)
  {
    if (ren == this->Renderers[cc].GetPointer())
    {
      pos[cc] += this->SliceIncrements[cc];
    }
  }
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, &pos);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::OnMouseWheelBackwardEvent()
{
  vtkRenderer* ren = this->Interactor->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);

  double pos[3];
  this->GetSlicePosition(pos);
  for (int cc = 0; cc < 3; cc++)
  {
    if (ren == this->Renderers[cc].GetPointer())
    {
      pos[cc] -= this->SliceIncrements[cc];
    }
  }
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, pos);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::MoveSlicePosition(vtkRenderer* ren, double position[3])
{
  int viewIndex = -1;
  for (int cc = 0; cc < 3; cc++)
  {
    if (ren == this->Renderers[cc].GetPointer())
    {
      viewIndex = cc;
      break;
    }
  }
  if (viewIndex != -1)
  {
    position[viewIndex] = this->SlicePosition[viewIndex];
    this->InvokeEvent(vtkCommand::PlacePointEvent, position);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetBackground(double r, double g, double b)
{
  this->Superclass::SetBackground(r, g, b);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetBackground(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetBackground2(double r, double g, double b)
{
  this->Superclass::SetBackground2(r, g, b);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetBackground2(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetBackgroundTexture(vtkTexture* val)
{
  this->Superclass::SetBackgroundTexture(val);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetBackgroundTexture(val);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetGradientBackground(int val)
{
  this->Superclass::SetGradientBackground(val);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetGradientBackground(val ? true : false);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetTexturedBackground(int val)
{
  this->Superclass::SetTexturedBackground(val);
  for (int cc = 0; cc < 3; cc++)
  {
    this->Renderers[cc]->SetTexturedBackground(val ? true : false);
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetCenterOfRotation(double x, double y, double z)
{
  this->Superclass::SetCenterOfRotation(x, y, z);
  this->OrthographicInteractorStyle->SetCenterOfRotation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetRotationFactor(double factor)
{
  this->Superclass::SetRotationFactor(factor);
  this->OrthographicInteractorStyle->SetRotationFactor(factor);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetGridAxes3DActor(vtkPVGridAxes3DActor* gridActor)
{
  vtkSmartPointer<vtkPVGridAxes3DActor> prev = this->GridAxes3DActor;
  this->Superclass::SetGridAxes3DActor(gridActor);
  if (prev == this->GridAxes3DActor)
  {
    // nothing changed.
    return;
  }

  if (prev && this->GridAxes3DActorObserverId)
  {
    prev->RemoveObserver(this->GridAxes3DActorObserverId);
  }
  if (this->GridAxes3DActor)
  {
    this->GridAxes3DActorObserverId = this->GridAxes3DActor->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkPVOrthographicSliceView::OnGridAxes3DActorModified);
    this->GridAxes3DActorsNeedShallowCopy = true;
  }

  // we currently don't support grid axes in tile-display mode.
  const bool in_tile_display_mode = this->InTileDisplayMode();
  for (int cc = 0; cc < 3; cc++)
  {
    if (this->GridAxes3DActors[cc])
    {
      this->Renderers[cc]->RemoveViewProp(this->GridAxes3DActors[cc]);
    }
    vtkPVGridAxes3DActor* clone = gridActor ? gridActor->NewInstance() : NULL;
    this->GridAxes3DActors[cc].TakeReference(clone);
    if (this->GridAxes3DActors[cc] && !in_tile_display_mode)
    {
      this->GridAxes3DActors[cc]->ShallowCopy(gridActor);
      this->Renderers[cc]->AddViewProp(this->GridAxes3DActors[cc]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::OnGridAxes3DActorModified()
{
  this->GridAxes3DActorsNeedShallowCopy = true;
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
