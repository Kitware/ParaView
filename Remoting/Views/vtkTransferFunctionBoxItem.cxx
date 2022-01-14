/*=========================================================================

  Program:   ParaView
  Module:    vtkTransferFunctionBoxItem.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionBoxItem.h"

#include "vtkBrush.h"
#include "vtkColorTransferFunction.h"
#include "vtkContext2D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransform2D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVectorOperators.h"

//-------------------------------------------------------------------------------------------------
namespace
{
inline bool PointIsWithinBounds2D(double point[2], double bounds[4], const double delta[2])
{
  if (!point || !bounds || !delta)
  {
    return false;
  }

  for (int i = 0; i < 2; i++)
  {
    if (point[i] + delta[i] < bounds[2 * i] || point[i] - delta[i] > bounds[2 * i + 1])
    {
      return false;
    }
  }
  return true;
}
}

//-------------------------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTransferFunctionBoxItem)

  //-------------------------------------------------------------------------------------------------
  vtkTransferFunctionBoxItem::vtkTransferFunctionBoxItem()
  : Superclass()
{
  this->ValidBounds[0] = 0.0;
  this->ValidBounds[1] = 1.0;
  this->ValidBounds[2] = 0.0;
  this->ValidBounds[3] = 1.0;
  // Initialize box, points are ordered as:
  //     3 ----- 2
  //     |       |
  // (4) 0 ----- 1
  this->AddPoint(0.0, 0.0);
  this->AddPoint(1.0, 0.0);
  this->AddPoint(1.0, 1.0);
  this->AddPoint(0.0, 1.0);

  // Point 0 is repeated for rendering purposes
  this->BoxPoints->InsertNextPoint(0.0, 0.0);

  // Initialize outline
  this->Pen->SetColor(63, 90, 115, 200);
  this->Pen->SetLineType(vtkPen::SOLID_LINE);
  this->Pen->SetWidth(1.);
  this->SelectedPointPen->SetColor(255, 0, 255, 200);
  this->SelectedPointPen->SetWidth(2.);

  // Initialize texture
  auto tex = this->Texture.GetPointer();
  const int texSize = 256;
  tex->SetDimensions(texSize, texSize, 1);
  tex->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  auto arr = vtkUnsignedCharArray::SafeDownCast(tex->GetPointData()->GetScalars());
  const auto dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, texSize * texSize * 4 * sizeof(unsigned char));
}

//-------------------------------------------------------------------------------------------------
vtkTransferFunctionBoxItem::~vtkTransferFunctionBoxItem() = default;

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::DragBox(const double deltaX, const double deltaY)
{
  this->StartChanges();

  if (!BoxIsWithinBounds(deltaX, deltaY))
    return;

  this->MovePoint(BOTTOM_LEFT, deltaX, deltaY);
  this->MovePoint(BOTTOM_LEFT_LOOP, deltaX, deltaY);
  this->MovePoint(BOTTOM_RIGHT, deltaX, deltaY);
  this->MovePoint(TOP_RIGHT, deltaX, deltaY);
  this->MovePoint(TOP_LEFT, deltaX, deltaY);

  this->EndChanges();
  this->InvokeEvent(vtkTransferFunctionBoxItem::BoxEditEvent);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::BoxIsWithinBounds(const double deltaX, const double deltaY)
{
  double bounds[4];
  this->GetValidBounds(bounds);

  const double delta[2] = { 0.0, 0.0 };
  for (vtkIdType id = 0; id < this->NumPoints; id++)
  {
    double pos[2];
    this->BoxPoints->GetPoint(id, pos);
    pos[0] += deltaX;
    pos[1] += deltaY;
    if (!PointIsWithinBounds2D(pos, bounds, delta))
      return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::MovePoint(
  vtkIdType pointId, const double deltaX, const double deltaY)
{
  double pos[2];
  this->BoxPoints->GetPoint(pointId, pos);

  double newPos[2] = { pos[0] + deltaX, pos[1] + deltaY };
  this->ClampToValidPosition(newPos);

  this->BoxPoints->SetPoint(pointId, newPos[0], newPos[1]);
}

//-------------------------------------------------------------------------------------------------
vtkIdType vtkTransferFunctionBoxItem::AddPoint(const double x, const double y)
{
  double pos[2] = { x, y };
  return this->AddPoint(pos);
}

//-------------------------------------------------------------------------------------------------
vtkIdType vtkTransferFunctionBoxItem::AddPoint(double* pos)
{
  if (this->BoxPoints->GetNumberOfPoints() >= 4)
  {
    return 3;
  }

  this->StartChanges();

  const vtkIdType id = this->BoxPoints->InsertNextPoint(pos[0], pos[1]);
  Superclass::AddPointId(id);

  this->EndChanges();

  return id;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::DragCorner(const vtkIdType cornerId, const double* delta)
{
  if (cornerId < 0 || cornerId > 3)
  {
    return;
  }

  this->StartChanges();

  // Move dragged corner and adjacent corners
  switch (cornerId)
  {
    case BOTTOM_LEFT:
      if (this->ArePointsCrossing(cornerId, delta, TOP_RIGHT))
        break;
      this->MovePoint(cornerId, delta[0], delta[1]);
      this->MovePoint(BOTTOM_LEFT_LOOP, delta[0], delta[1]);
      this->MovePoint(TOP_LEFT, delta[0], 0.0);
      this->MovePoint(BOTTOM_RIGHT, 0.0, delta[1]);
      break;

    case BOTTOM_RIGHT:
      if (this->ArePointsCrossing(cornerId, delta, TOP_LEFT))
        break;
      this->MovePoint(cornerId, delta[0], delta[1]);
      this->MovePoint(BOTTOM_LEFT, 0.0, delta[1]);
      this->MovePoint(BOTTOM_LEFT_LOOP, 0.0, delta[1]);
      this->MovePoint(TOP_RIGHT, delta[0], 0.0);
      break;

    case TOP_RIGHT:
      if (this->ArePointsCrossing(cornerId, delta, BOTTOM_LEFT))
        break;
      this->MovePoint(cornerId, delta[0], delta[1]);
      this->MovePoint(BOTTOM_RIGHT, delta[0], 0.0);
      this->MovePoint(TOP_LEFT, 0.0, delta[1]);
      break;

    case TOP_LEFT:
      if (this->ArePointsCrossing(cornerId, delta, BOTTOM_RIGHT))
        break;
      this->MovePoint(cornerId, delta[0], delta[1]);
      this->MovePoint(TOP_RIGHT, 0.0, delta[1]);
      this->MovePoint(BOTTOM_LEFT, delta[0], 0.0);
      this->MovePoint(BOTTOM_LEFT_LOOP, delta[0], 0.0);
      break;
  }

  this->EndChanges();
  this->InvokeEvent(vtkTransferFunctionBoxItem::BoxEditEvent);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::ArePointsCrossing(
  const vtkIdType pointA, const double* deltaA, const vtkIdType pointB)
{
  double posA[2];
  this->BoxPoints->GetPoint(pointA, posA);

  double posB[2];
  this->BoxPoints->GetPoint(pointB, posB);

  const double distXBefore = posA[0] - posB[0];
  const double distXAfter = posA[0] + deltaA[0] - posB[0];
  if (distXAfter * distXBefore <= 0.0) // Sign changed
  {
    return true;
  }

  const double distYBefore = posA[1] - posB[1];
  const double distYAfter = posA[1] + deltaA[1] - posB[1];
  if (distYAfter * distYBefore <= 0.0) // Sign changed
  {
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------
vtkIdType vtkTransferFunctionBoxItem::RemovePoint(double* vtkNotUsed(pos))
{
  // This method does nothing as this item has a fixed number of points (4).
  return 0;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::SetControlPoint(
  vtkIdType vtkNotUsed(index), double* vtkNotUsed(point))
{
  // This method does nothing as this item has a fixed number of points (4).
  return;
}

//-------------------------------------------------------------------------------------------------
vtkIdType vtkTransferFunctionBoxItem::GetNumberOfPoints() const
{
  return static_cast<vtkIdType>(this->NumPoints);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::GetControlPoint(vtkIdType index, double* point) const
{
  if (index >= this->NumPoints)
  {
    return;
  }

  this->BoxPoints->GetPoint(index, point);
}

//-------------------------------------------------------------------------------------------------
vtkMTimeType vtkTransferFunctionBoxItem::GetControlPointsMTime()
{
  return this->GetMTime();
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::emitEvent(unsigned long event, void* params)
{
  this->InvokeEvent(event, params);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::Paint(vtkContext2D* painter)
{
  // Prepare brush
  if (!this->Initialized)
  {
    this->ComputeTexture();
    this->Initialized = true;
    this->InvokeEvent(vtkTransferFunctionBoxItem::BoxAddEvent);
  }

  auto brush = painter->GetBrush();
  brush->SetColorF(0.0, 0.0, 0.0, 0.0);
  brush->SetTexture(this->Texture.GetPointer());
  brush->SetTextureProperties(vtkBrush::Linear | vtkBrush::Stretch);

  // Prepare outline
  painter->ApplyPen(this->Pen.GetPointer());

  if (!this->Selected)
  {
    painter->DrawPolygon(this->BoxPoints.GetPointer());
  }
  else
  {
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->ApplyPen(this->SelectedPointPen);
    painter->DrawPolygon(this->BoxPoints.GetPointer());
  }
  return Superclass::Paint(painter);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::ComputeTexture()
{
  double colorC[3] = { 1.0, 1.0, 1.0 };
  colorC[0] = this->Color[0] * 255;
  colorC[1] = this->Color[1] * 255;
  colorC[2] = this->Color[2] * 255;

  double amp = this->Alpha * 255;
  double sigma = 80.0;
  double expdenom = 2 * sigma * sigma;

  auto arr = vtkUnsignedCharArray::SafeDownCast(this->Texture->GetPointData()->GetScalars());

  for (vtkIdType i = 0; i < 256; ++i)
  {
    for (vtkIdType j = 0; j < 256; ++j)
    {
      double color[4] = { colorC[0], colorC[1], colorC[2], 0.0 };
      double e = -1.0 * ((i - 128) * (i - 128) + (j - 128) * (j - 128)) / expdenom;
      color[3] = amp * std::exp(e);
      arr->SetTuple(i * 256 + j, color);
    }
  }
  this->Texture->Modified();
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::Hit(const vtkContextMouseEvent& mouse)
{
  vtkVector2f vpos = mouse.GetPos();
  this->TransformScreenToData(vpos, vpos);

  double pos[2];
  pos[0] = vpos.GetX();
  pos[1] = vpos.GetY();

  double bounds[4];
  this->GetBounds(bounds);

  const double delta[2] = { 0.0, 0.0 };
  const bool isWithinBox = PointIsWithinBounds2D(pos, bounds, delta);

  // maybe the cursor is over the first or last point (which could be outside
  // the bounds because of the screen point size).
  bool isOverPoint = false;
  for (int i = 0; i < this->NumPoints; ++i)
  {
    isOverPoint = this->IsOverPoint(pos, i);
    if (isOverPoint)
    {
      break;
    }
  }

  return isWithinBox || isOverPoint;
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  this->MouseMoved = false;
  this->PointToToggle = -1;

  vtkVector2f vpos = mouse.GetPos();
  this->TransformScreenToData(vpos, vpos);
  double pos[2];
  pos[0] = vpos.GetX();
  pos[1] = vpos.GetY();
  vtkIdType pointUnderMouse = this->FindBoxPoint(pos);

  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    this->SelectBox();
    if (pointUnderMouse != -1)
    {
      this->SetCurrentPoint(pointUnderMouse);
    }
    else
    {
      this->SetCurrentPoint(-1);
    }
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  return Superclass::MouseButtonReleaseEvent(mouse);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  return Superclass::MouseDoubleClickEvent(mouse);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  switch (mouse.GetButton())
  {
    case vtkContextMouseEvent::LEFT_BUTTON:
      if (this->CurrentPoint == -1)
      {
        // Drag box
        vtkVector2f deltaPos = mouse.GetPos() - mouse.GetLastPos();
        this->DragBox(deltaPos.GetX(), deltaPos.GetY());
        this->Scene->SetDirty(true);
        return true;
      }
      else
      {
        // Drag corner
        vtkVector2d deltaPos = (mouse.GetPos() - mouse.GetLastPos()).Cast<double>();
        this->DragCorner(this->CurrentPoint, deltaPos.GetData());
        this->Scene->SetDirty(true);
        return true;
      }
      break;

    default:
      break;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::ClampToValidPosition(double pos[2])
{
  double bounds[4];
  this->GetValidBounds(bounds);
  pos[0] = vtkMath::ClampValue(pos[0], bounds[0], bounds[1]);
  pos[1] = vtkMath::ClampValue(pos[1], bounds[2], bounds[3]);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::KeyPressEvent(const vtkContextKeyEvent& key)
{
  if (key.GetInteractor()->GetKeySym() == std::string("Delete") ||
    key.GetInteractor()->GetKeySym() == std::string("BackSpace"))
  {
    this->InvokeEvent(vtkTransferFunctionBoxItem::BoxDeleteEvent);
  }
  return Superclass::KeyPressEvent(key);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionBoxItem::KeyReleaseEvent(const vtkContextKeyEvent& key)
{
  return Superclass::KeyPressEvent(key);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Box [x, y, width, height]: [" << this->Box.GetX() << ", " << this->Box.GetY()
     << ", " << this->Box.GetWidth() << ", " << this->Box.GetHeight() << "]\n";
}

//-------------------------------------------------------------------------------------------------
const vtkRectd& vtkTransferFunctionBoxItem::GetBox()
{
  double lowerBound[2];
  this->BoxPoints->GetPoint(BOTTOM_LEFT, lowerBound);

  double upperBound[2];
  this->BoxPoints->GetPoint(TOP_RIGHT, upperBound);

  const double width = upperBound[0] - lowerBound[0];
  const double height = upperBound[1] - lowerBound[1];

  this->Box.Set(lowerBound[0], lowerBound[1], width, height);

  return this->Box;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::SetBox(
  const double x, const double y, const double width, const double height)
{
  this->StartChanges();
  double pos[2];
  pos[0] = x;
  pos[1] = y;

  this->ClampToValidPosition(pos);
  this->BoxPoints->SetPoint(0, pos);
  this->BoxPoints->SetPoint(4, pos);

  pos[0] = x + width;
  pos[1] = y;
  this->ClampToValidPosition(pos);
  this->BoxPoints->SetPoint(1, pos);

  pos[0] = x + width;
  pos[1] = y + height;
  this->ClampToValidPosition(pos);
  this->BoxPoints->SetPoint(2, pos);

  pos[0] = x;
  pos[1] = y + height;
  this->ClampToValidPosition(pos);
  this->BoxPoints->SetPoint(3, pos);

  this->EndChanges();
  this->InvokeEvent(vtkTransferFunctionBoxItem::BoxEditEvent);
}

vtkIdType vtkTransferFunctionBoxItem::FindBoxPoint(double* _pos)
{
  vtkVector2f vpos(_pos[0], _pos[1]);
  this->TransformDataToScreen(vpos, vpos);
  double pos[2] = { vpos.GetX(), vpos.GetY() };

  double tolerance = 1.3;
  double radius2 = this->ScreenPointRadius * this->ScreenPointRadius * tolerance * tolerance;

  double screenPos[2];
  this->ControlPointsTransform->TransformPoints(pos, screenPos, 1);
  vtkIdType pointId = -1;
  double minDist = VTK_DOUBLE_MAX;
  const int numberOfPoints = this->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    double point[4];
    this->GetControlPoint(i, point);
    vtkVector2f vpos1(point[0], point[1]);
    this->TransformDataToScreen(vpos1, vpos1);
    point[0] = vpos1.GetX();
    point[1] = vpos1.GetY();

    double screenPoint[2];
    this->ControlPointsTransform->TransformPoints(point, screenPoint, 1);
    double distance2 = (screenPoint[0] - screenPos[0]) * (screenPoint[0] - screenPos[0]) +
      (screenPoint[1] - screenPos[1]) * (screenPoint[1] - screenPos[1]);

    if (distance2 <= radius2)
    {
      if (distance2 == 0.)
      { // we found the best match ever
        return i;
      }
      else if (distance2 < minDist)
      { // we found something not too bad, maybe
        // we can find closer
        pointId = i;
        minDist = distance2;
      }
    }
  }
  return pointId;
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkTransferFunctionBoxItem::GetTexture() const
{
  return this->Texture;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::SetValidBounds(double x0, double x1, double y0, double y1)
{
  if (this->ValidBounds[0] == x0 && this->ValidBounds[1] == x1 && this->ValidBounds[2] == y0 &&
    this->ValidBounds[3] == y1)
  {
    return;
  }

  for (vtkIdType id = 0; id < this->NumPoints; id++)
  {
    double pos[2];
    this->BoxPoints->GetPoint(id, pos);
    pos[0] =
      (pos[0] - this->ValidBounds[0]) * (x1 - x0) / (this->ValidBounds[1] - this->ValidBounds[0]) +
      x0;
    pos[1] =
      (pos[1] - this->ValidBounds[2]) * (y1 - y0) / (this->ValidBounds[3] - this->ValidBounds[2]) +
      y0;
    this->BoxPoints->SetPoint(id, pos);
  }
  this->Superclass::SetValidBounds(x0, x1, y0, y1);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionBoxItem::SelectBox()
{
  if (this->Selected)
  {
    return;
  }
  this->SetSelected(true);
  this->Scene->SetDirty(true);
  this->InvokeEvent(vtkTransferFunctionBoxItem::BoxSelectEvent);
}
