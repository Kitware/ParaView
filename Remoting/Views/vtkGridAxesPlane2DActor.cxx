/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxesPlane2DActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridAxesPlane2DActor.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkGridAxesPlane2DActor);
//----------------------------------------------------------------------------
vtkGridAxesPlane2DActor* vtkGridAxesPlane2DActor::New(vtkGridAxesHelper* helper)
{
  vtkGridAxesPlane2DActor* self = new vtkGridAxesPlane2DActor(helper);
  self->InitializeObjectBase();
  return self;
}

//----------------------------------------------------------------------------
vtkGridAxesPlane2DActor::vtkGridAxesPlane2DActor(vtkGridAxesHelper* helper)
  : Face(vtkGridAxesPlane2DActor::MIN_YZ)
  , GenerateGrid(true)
  , GenerateEdges(true)
  , GenerateTicks(true)
  , TickDirection(vtkGridAxesPlane2DActor::TICK_DIRECTION_BOTH)
  , Helper(helper)
  , HelperManagedExternally(helper != NULL)
{
  if (helper == NULL)
  {
    this->Helper = vtkSmartPointer<vtkGridAxesHelper>::New();
    this->GridBounds[0] = this->GridBounds[2] = this->GridBounds[4] = -1.0;
    this->GridBounds[1] = this->GridBounds[3] = this->GridBounds[5] = 1.0;
  }
  else
  {
    vtkMath::UninitializeBounds(this->GridBounds);
    // So we can warn if user changes the bounds when they are not being used.
  }

  this->Mapper->SetInputDataObject(this->PolyData.GetPointer());
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetRepresentationToWireframe();
}

//----------------------------------------------------------------------------
vtkGridAxesPlane2DActor::~vtkGridAxesPlane2DActor()
{
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::GetActors(vtkPropCollection* props)
{
  if (this->GetVisibility())
  {
    vtkViewport* vp = nullptr;
    if (this->NumberOfConsumers)
    {
      vp = vtkViewport::SafeDownCast(this->Consumers[0]);
      if (vp)
      {
        this->UpdateGeometry(vp);
      }
    }
  }

  props->AddItem(this->Actor.Get());
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::SetProperty(vtkProperty* property)
{
  if (this->GetProperty() != property)
  {
    this->Actor->SetProperty(property);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkProperty* vtkGridAxesPlane2DActor::GetProperty()
{
  return this->Actor->GetProperty();
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::SetTickPositions(int index, vtkDoubleArray* data)
{
  assert(index >= 0 && index < 3 && (data == NULL || data->GetNumberOfComponents() <= 1));
  if (data == NULL)
  {
    if (this->TickPositions[index].size() != 0)
    {
      this->TickPositions[index].clear();
      this->Modified();
    }
  }
  else if (static_cast<vtkIdType>(this->TickPositions[index].size()) != data->GetNumberOfTuples() ||
    std::equal(data->GetPointer(0), data->GetPointer(0) + data->GetNumberOfTuples(),
      this->TickPositions[index].begin()) == false)
  {
    this->TickPositions[index].resize(data->GetNumberOfTuples());
    std::copy(data->GetPointer(0), data->GetPointer(0) + data->GetNumberOfTuples(),
      this->TickPositions[index].begin());
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesPlane2DActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  // Do tasks that need to be done when this->MTime changes.
  this->Update(viewport);
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::UpdateGeometry(vtkViewport* viewport)
{
  // Do tasks that need to be done when this->MTime changes.
  this->Update(viewport);
}

//----------------------------------------------------------------------------
int vtkGridAxesPlane2DActor::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}

//----------------------------------------------------------------------------
int vtkGridAxesPlane2DActor::RenderOverlay(vtkViewport* viewport)
{
  return this->Actor->RenderOverlay(viewport);
}

//----------------------------------------------------------------------------
int vtkGridAxesPlane2DActor::HasTranslucentPolygonalGeometry()
{
  return this->Actor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::Update(vtkViewport* viewport)
{
  if (this->HelperManagedExternally == false)
  {
    this->Helper->SetGridBounds(this->GridBounds);
    this->Helper->SetFace(this->Face);
    this->Helper->SetMatrix(this->GetMatrix());
    this->Helper->UpdateForViewport(viewport);
  }
  else
  {
    assert(vtkMath::AreBoundsInitialized(this->GridBounds) == 0);
  }

  this->PolyData->Initialize();
  this->LineSegments.clear();

  bool success = true;
  success = success && (this->GenerateEdges == false || this->UpdateEdges(viewport));
  success = success && (this->GenerateGrid == false || this->UpdateGrid(viewport));
  success = success && (this->GenerateTicks == false || this->UpdateTicks(viewport));
  if (!success)
  {
    return;
  }

  this->PolyDataPoints->Allocate(this->LineSegments.size() * 2);
  this->PolyDataPoints->SetDataType(VTK_FLOAT);
  this->PolyDataLines->Allocate(
    this->PolyDataLines->EstimateSize(static_cast<vtkIdType>(this->LineSegments.size()), 2));
  for (std::deque<LineSegmentType>::iterator iter = this->LineSegments.begin(),
                                             max = this->LineSegments.end();
       iter != max; ++iter)
  {
    vtkIdType pids[2];
    pids[0] = this->PolyDataPoints->InsertNextPoint(iter->first.GetData());
    pids[1] = this->PolyDataPoints->InsertNextPoint(iter->second.GetData());
    this->PolyDataLines->InsertNextCell(2, pids);
  }
  this->PolyData->SetPoints(this->PolyDataPoints.GetPointer());
  this->PolyData->SetLines(this->PolyDataLines.GetPointer());
  this->PolyDataPoints->Modified();
  this->PolyDataLines->Modified();
  this->PolyData->Modified();
  this->LineSegments.clear();

  this->Actor->SetUserMatrix(this->GetMatrix());
}

//----------------------------------------------------------------------------
bool vtkGridAxesPlane2DActor::UpdateEdges(vtkViewport*)
{
  assert(this->GenerateEdges);
  const vtkTuple<vtkVector3d, 4>& gridPoints = this->Helper->GetPoints();
  for (int cc = 0; cc < 4; cc++)
  {
    this->LineSegments.push_back(LineSegmentType(gridPoints[cc], gridPoints[(cc + 1) % 4]));
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkGridAxesPlane2DActor::UpdateGrid(vtkViewport*)
{
  assert(this->GenerateGrid);
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<vtkVector3d, 4>& gridPoints = this->Helper->GetPoints();

  for (int cc = 0; cc < 2; cc++)
  {
    vtkVector3d points[2];
    points[0] = gridPoints[0];
    points[1] = gridPoints[cc == 0 ? 3 : 1];

    const std::deque<double>& tick_positions = this->TickPositions[activeAxes[cc]];
    for (std::deque<double>::const_iterator iter = tick_positions.begin();
         iter != tick_positions.end(); ++iter)
    {
      points[0][activeAxes[cc]] = *iter;
      points[1][activeAxes[cc]] = *iter;
      this->LineSegments.push_back(LineSegmentType(points[0], points[1]));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkGridAxesPlane2DActor::UpdateTicks(vtkViewport* viewport)
{
  assert(this->GenerateTicks);
  const vtkTuple<vtkVector3d, 4>& gridPoints = this->Helper->GetPoints();
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<vtkVector2d, 4>& viewportNormals = this->Helper->GetViewportNormals();
  const vtkTuple<vtkVector2d, 4>& viewportPoints = this->Helper->GetViewportPointsAsDouble();

  vtkNew<vtkMatrix4x4> inverted;
  vtkMatrix4x4::Invert(this->GetMatrix(), inverted.GetPointer());

  double offsets[4];
  vtkNew<vtkCoordinate> coordinate;

  vtkWindow* renWin = viewport->GetVTKWindow();
  int tileScale[2];
  renWin->GetTileScale(tileScale);

  for (int cc = 0; cc < 4; cc++)
  {
    vtkVector2d normal = viewportNormals[cc];

    coordinate->SetCoordinateSystemToViewport();
    coordinate->SetValue(viewportPoints[cc].GetX(), viewportPoints[cc].GetY());
    const double* value = coordinate->GetComputedWorldValue(viewport);
    vtkVector3d pw1(value);

    vtkVector2d pt2 = viewportPoints[cc] + normal * 10;
    coordinate->SetValue(pt2.GetX(), pt2.GetY());
    value = coordinate->GetComputedWorldValue(viewport);
    vtkVector3d pw2(value);
    offsets[cc] = (pw2 - pw1).Norm() * tileScale[0];
    // FIXME: make this better -- maybe use average?
  }

  for (int cc = 0; cc < 4; cc++)
  {
    if (this->Helper->GetLabelVisibilities()[cc] == false)
    {
      continue;
    }

    vtkVector3d points[2];
    points[0] = gridPoints[cc];

    // FIXME: this can be precomputed.
    vtkVector3d direction = gridPoints[(cc + 1) % 4] - gridPoints[cc];
    vtkVector3d next = gridPoints[(cc + 2) % 4] - gridPoints[(cc + 1) % 4];
    vtkVector3d normal = direction.Cross(direction.Cross(next)).Normalized();

    double n[] = { normal[0], normal[1], normal[2], 0.0 };
    inverted->MultiplyPoint(n, n);
    normal[0] = n[0];
    normal[1] = n[1];
    normal[2] = n[2];

    // We need to compute the length of the tick.
    points[1] = ((this->TickDirection & TICK_DIRECTION_OUTWARDS) != 0)
      ? points[0] + normal * offsets[cc]
      : points[0];

    points[0] = ((this->TickDirection & TICK_DIRECTION_INWARDS) != 0)
      ? points[0] - normal * offsets[cc]
      : points[0];

    const std::deque<double>& tick_positions = this->TickPositions[activeAxes[cc % 2]];
    for (std::deque<double>::const_iterator iter = tick_positions.begin();
         iter != tick_positions.end(); ++iter)
    {
      points[0][activeAxes[cc % 2]] = *iter;
      points[1][activeAxes[cc % 2]] = *iter;
      this->LineSegments.push_back(LineSegmentType(points[0], points[1]));
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkGridAxesPlane2DActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
