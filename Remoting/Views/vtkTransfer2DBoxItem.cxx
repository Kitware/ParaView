/*=========================================================================

  Program:   ParaView
  Module:    vtkTransfer2DBoxItem.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransfer2DBoxItem.h"

#include "vtkBrush.h"
#include "vtkColorTransferFunction.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkTransform2D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVectorOperators.h"

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

////////////////////////////////////////////////////////////////////////////////
vtkStandardNewMacro(vtkTransfer2DBoxItem)

  vtkTransfer2DBoxItem::vtkTransfer2DBoxItem()
  : Superclass()
{
  // Initialize box, points are ordered as:
  //     3 ----- 2
  //     |       |
  // (4) 0 ----- 1
  this->AddPoint(1.0, 1.0);
  this->AddPoint(20.0, 1.0);
  this->AddPoint(20.0, 20.0);
  this->AddPoint(1.0, 20.0);

  // Point 0 is repeated for rendering purposes
  this->BoxPoints->InsertNextPoint(1.0, 1.0);

  // Initialize outline
  this->Pen->SetWidth(2.);
  this->Pen->SetColor(255, 255, 255);
  this->Pen->SetLineType(vtkPen::SOLID_LINE);

  // Initialize texture
  auto tex = this->Texture.GetPointer();
  const int texSize = 256;
  tex->SetDimensions(texSize, texSize, 1);
  tex->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  auto arr = vtkUnsignedCharArray::SafeDownCast(tex->GetPointData()->GetScalars());
  const auto dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, texSize * texSize * 4 * sizeof(unsigned char));
}

vtkTransfer2DBoxItem::~vtkTransfer2DBoxItem() = default;

void vtkTransfer2DBoxItem::DragBox(const double deltaX, const double deltaY)
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
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

bool vtkTransfer2DBoxItem::BoxIsWithinBounds(const double deltaX, const double deltaY)
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

void vtkTransfer2DBoxItem::MovePoint(vtkIdType pointId, const double deltaX, const double deltaY)
{
  double pos[2];
  this->BoxPoints->GetPoint(pointId, pos);

  double newPos[2] = { pos[0] + deltaX, pos[1] + deltaY };
  this->ClampToValidPosition(newPos);

  this->BoxPoints->SetPoint(pointId, newPos[0], newPos[1]);
}

vtkIdType vtkTransfer2DBoxItem::AddPoint(const double x, const double y)
{
  double pos[2] = { x, y };
  return this->AddPoint(pos);
}

vtkIdType vtkTransfer2DBoxItem::AddPoint(double* pos)
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

void vtkTransfer2DBoxItem::DragCorner(const vtkIdType cornerId, const double* delta)
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
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

bool vtkTransfer2DBoxItem::ArePointsCrossing(
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

vtkIdType vtkTransfer2DBoxItem::RemovePoint(double* vtkNotUsed(pos))
{
  // This method does nothing as this item has a fixed number of points (4).
  return 0;
}

void vtkTransfer2DBoxItem::SetControlPoint(vtkIdType vtkNotUsed(index), double* vtkNotUsed(point))
{
  // This method does nothing as this item has a fixed number of points (4).
  return;
}

vtkIdType vtkTransfer2DBoxItem::GetNumberOfPoints() const
{
  return static_cast<vtkIdType>(this->NumPoints);
}

void vtkTransfer2DBoxItem::GetControlPoint(vtkIdType index, double* point) const
{
  if (index >= this->NumPoints)
  {
    return;
  }

  this->BoxPoints->GetPoint(index, point);
}

vtkMTimeType vtkTransfer2DBoxItem::GetControlPointsMTime()
{
  return this->GetMTime();
}

void vtkTransfer2DBoxItem::emitEvent(unsigned long event, void* params)
{
  this->InvokeEvent(event, params);
}

bool vtkTransfer2DBoxItem::Paint(vtkContext2D* painter)
{
  // Prepare brush
  if (!this->Initialized)
  {
    this->ComputeTexture();
    this->Initialized = true;
  }

  auto brush = painter->GetBrush();
  brush->SetColorF(0.0, 0.0, 0.0, 0.0);
  brush->SetTexture(this->Texture.GetPointer());
  brush->SetTextureProperties(vtkBrush::Linear | vtkBrush::Stretch);

  // Prepare outline
  painter->ApplyPen(this->Pen.GetPointer());

  painter->DrawPolygon(this->BoxPoints.GetPointer());
  return Superclass::Paint(painter);
}

void vtkTransfer2DBoxItem::ComputeTexture()
{
  double colorC[3] = { 255 * vtkMath::Random(), 255 * vtkMath::Random(), 255 * vtkMath::Random() };
  auto arr = vtkUnsignedCharArray::SafeDownCast(this->Texture->GetPointData()->GetScalars());

  for (vtkIdType i = 0; i < 256; ++i)
  {
    for (vtkIdType j = 0; j < 256; ++j)
    {
      double color[4] = { colorC[0], colorC[1], colorC[2], 0.0 };
      double sigma = 80.0;
      double e = -1.0 * ((i - 128) * (i - 128) + (j - 128) * (j - 128)) / (2 * sigma * sigma);
      double amp = 255.0;
      color[3] = amp * std::exp(e);
      arr->SetTuple(i * 256 + j, color);
    }
  }
  this->Texture->Modified();
}

bool vtkTransfer2DBoxItem::Hit(const vtkContextMouseEvent& mouse)
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

bool vtkTransfer2DBoxItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
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
    if (pointUnderMouse != -1)
    {
      this->SetCurrentPoint(pointUnderMouse);
      return true;
    }
    else
    {
      this->SetCurrentPoint(-1);
    }
    return true;
  }

  return false;
}

bool vtkTransfer2DBoxItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  return Superclass::MouseButtonReleaseEvent(mouse);
}

bool vtkTransfer2DBoxItem::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  return Superclass::MouseDoubleClickEvent(mouse);
}

bool vtkTransfer2DBoxItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
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

void vtkTransfer2DBoxItem::ClampToValidPosition(double pos[2])
{
  double bounds[4];
  this->GetValidBounds(bounds);
  pos[0] = vtkMath::ClampValue(pos[0], bounds[0], bounds[1]);
  pos[1] = vtkMath::ClampValue(pos[1], bounds[2], bounds[3]);
}

bool vtkTransfer2DBoxItem::KeyPressEvent(const vtkContextKeyEvent& key)
{
  return Superclass::KeyPressEvent(key);
}

bool vtkTransfer2DBoxItem::KeyReleaseEvent(const vtkContextKeyEvent& key)
{
  return Superclass::KeyPressEvent(key);
}

void vtkTransfer2DBoxItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Box [x, y, width, height]: [" << this->Box.GetX() << ", " << this->Box.GetY()
     << ", " << this->Box.GetWidth() << ", " << this->Box.GetHeight() << "]\n";
}

const vtkRectd& vtkTransfer2DBoxItem::GetBox()
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

void vtkTransfer2DBoxItem::SetBox(
  const double x, const double y, const double width, const double height)
{
  // Delta position
  double posBottomLeft[2];
  this->BoxPoints->GetPoint(BOTTOM_LEFT, posBottomLeft);

  vtkVector2f deltaPos(x - posBottomLeft[0], y - posBottomLeft[1]);
  this->TransformDataToScreen(deltaPos, deltaPos);

  // Delta dimensions
  double posTopRight[2];
  this->BoxPoints->GetPoint(TOP_RIGHT, posTopRight);

  double deltaSize[2];
  deltaSize[0] = width - (posTopRight[0] - posBottomLeft[0]);
  deltaSize[1] = height - (posTopRight[1] - posBottomLeft[1]);

  this->DragBox(deltaPos.GetX(), deltaPos.GetY());
  this->DragCorner(TOP_RIGHT, deltaSize);
}

// bool vtkTransfer2DBoxItem::NeedsTextureUpdate()
//{
//  auto tex = this->Texture.GetPointer();
//  return (tex->GetMTime() < this->ColorFunction->GetMTime() ||
//    tex->GetMTime() < this->OpacityFunction->GetMTime());
//}
//
// bool vtkTransfer2DBoxItem::IsInitialized()
//{
//  return this->ColorFunction && this->OpacityFunction;
//}

vtkIdType vtkTransfer2DBoxItem::FindBoxPoint(double* _pos)
{
  vtkVector2f vpos(_pos[0], _pos[1]);
  this->TransformDataToScreen(vpos, vpos);
  double pos[2] = { vpos.GetX(), vpos.GetY() };

  double tolerance = 1.3;
  double radius2 = this->ScreenPointRadius * this->ScreenPointRadius * tolerance * tolerance;

  double screenPos[2];
  this->Transform->TransformPoints(pos, screenPos, 1);
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
    this->Transform->TransformPoints(point, screenPoint, 1);
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
//
// void vtkTransfer2DBoxItem::rasterTransferFunction2DBox(vtkImageData* histogram2D,
//  const vtkRectd& box, vtkImageData* transferFunction, vtkColorTransferFunction* colorFunc,
//  vtkPiecewiseFunction* opacFunc)
//{
//  if (!histogram2D)
//  {
//    std::cerr << "Invalid histogram" << std::endl;
//    return;
//  }
//  if (!transferFunction)
//  {
//    std::cerr << "Invalid output image" << std::endl;
//    return;
//  }
//  if (!opacFunc || !colorFunc)
//  {
//    std::cerr << "Invalid transfer functions!" << std::endl;
//    return;
//  }
//
//  int bins[3];
//  transferFunction->GetDimensions(bins);
//
//  // If the transfer function image is uninitialized, initialize it
//  if (bins[0] == 0 && bins[1] == 0)
//  {
//    histogram2D->GetDimensions(bins);
//    transferFunction->SetDimensions(bins[0], bins[1], 1);
//    transferFunction->AllocateScalars(VTK_FLOAT, 4);
//  }
//
//  double spacing[3];
//  histogram2D->GetSpacing(spacing);
//  vtkIdType width = static_cast<vtkIdType>(box.GetWidth() / spacing[0]);
//  vtkIdType height = static_cast<vtkIdType>(box.GetHeight() / spacing[1]);
//
//  if (width <= 0 || height <= 0)
//  {
//    return;
//  }
//
//  // Assume color and opacity share the same data range
//  double range[2];
//  colorFunc->GetRange(range);
//
//  double* dataRGB = new double[width * 3];
//  colorFunc->GetTable(range[0], range[1], width, dataRGB);
//
//  double* dataAlpha = new double[width];
//  opacFunc->GetTable(range[0], range[1], width, dataAlpha);
//
//  // Copy the values into Transfer2D
//  vtkFloatArray* transfer =
//    vtkFloatArray::SafeDownCast(transferFunction->GetPointData()->GetScalars());
//
//  const vtkIdType x0 = static_cast<vtkIdType>(box.GetX() / spacing[0]);
//  const vtkIdType y0 = static_cast<vtkIdType>(box.GetY() / spacing[1]);
//
//  // The cast to vtkIdType is to help template type deduction.
//  width = vtkMath::ClampValue(width, static_cast<vtkIdType>(0), bins[0] - x0);
//  height = vtkMath::ClampValue(height, static_cast<vtkIdType>(0), bins[1] - y0);
//
//  for (vtkIdType j = 0; j < height; j++)
//    for (vtkIdType i = 0; i < width; i++)
//    {
//      double color[4];
//
//      color[0] = dataRGB[i * 3];
//      color[1] = dataRGB[i * 3 + 1];
//      color[2] = dataRGB[i * 3 + 2];
//      color[3] = dataAlpha[i];
//
//      const vtkIdType index = (y0 + j) * bins[1] + (x0 + i);
//      transfer->SetTuple(index, color);
//    }
//
//  delete[] dataRGB;
//  delete[] dataAlpha;
//}
