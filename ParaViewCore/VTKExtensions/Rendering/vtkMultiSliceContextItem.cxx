/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiSliceContextItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMultiSliceContextItem.h"

#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkRect.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <vector>

// ******************************* Internal class *****************************
struct vtkMultiSliceContextItem::vtkInternal
{
  int ActiveSize;
  int EdgeMargin;
  vtkNew<vtkAxis> Axis;
  vtkRectf ActiveArea;
  int ActiveSliceIndex;
  std::vector<double> Slices;
  std::vector<bool> SlicesVisibility;
  double LengthForRatio;
  // We assume that we won't have more than 255 slices by axis
  double VisibleSliceValues[255];

  // ----

  vtkInternal(vtkMultiSliceContextItem* parent)
  {
    this->ActiveSize = 10;
    this->EdgeMargin = 15;
    this->ActiveArea.Set(0, 0, 0, 0);
    this->ActiveSliceIndex = -1; // Nothing is selected for now
    parent->AddItem(this->Axis.GetPointer());
  }

  // ----

  bool IsInActiveArea(float x, float y)
  {
    return x >= this->ActiveArea.GetX() && y >= this->ActiveArea.GetY() &&
      x <= (this->ActiveArea.GetX() + this->ActiveArea.GetWidth()) &&
      y <= (this->ActiveArea.GetY() + this->ActiveArea.GetHeight());
  }

  // Return -1 if not found
  int FindSliceIndex(double value, double epsilon)
  {
    double min = value - epsilon;
    double max = value + epsilon;
    size_t size = this->Slices.size();
    for (size_t index = 0; index < size; index++)
    {
      if (this->Slices[index] >= min && this->Slices[index] <= max)
      {
        return static_cast<int>(index);
      }
    }
    return -1;
  }

  // ----

  double* UpdateVisibleValues(int& size)
  {
    std::vector<bool>::iterator visIter = this->SlicesVisibility.begin();
    std::vector<double>::iterator valueIter = this->Slices.begin();
    for (size = 0; valueIter != this->Slices.end() && size < 255; valueIter++, visIter++)
    {
      if (*visIter)
      {
        this->VisibleSliceValues[size] = *valueIter;
        size++;
      }
    }
    return this->VisibleSliceValues;
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiSliceContextItem);
//-----------------------------------------------------------------------------
vtkMultiSliceContextItem::vtkMultiSliceContextItem()
{
  this->Internal = new vtkInternal(this);
}

//-----------------------------------------------------------------------------
vtkMultiSliceContextItem::~vtkMultiSliceContextItem()
{
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::Paint(vtkContext2D* painter)
{
  // short var name for internal vars
  vtkRectf* activeArea = &this->Internal->ActiveArea;
  vtkAxis* axis = this->Internal->Axis.GetPointer();
  int activeSize = this->Internal->ActiveSize;
  int edgeMargin = this->Internal->EdgeMargin;
  int sliceHandleMargin = 2;

  // Local vars
  int width = this->GetScene()->GetViewWidth();
  int height = this->GetScene()->GetViewHeight();
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  float slicerX1 = 0.f, slicerY1 = 0.f, slicerX2 = 0.f, slicerY2 = 0.f;
  slicerX2 = slicerY2 = activeSize;

  // Reset active area
  activeArea->Set(0, 0, width, height);

  switch (axis->GetPosition())
  {
    case vtkAxis::LEFT:
      activeArea->SetX(width - activeSize);
      slicerX2 = width - sliceHandleMargin;
      slicerX1 = x1 = x2 = (width - activeSize);
      y1 = edgeMargin;
      y2 = height - edgeMargin;
      activeArea->SetWidth(activeSize);
      this->Internal->LengthForRatio = height - edgeMargin - edgeMargin;
      break;
    case vtkAxis::RIGHT:
      slicerX2 = sliceHandleMargin;
      slicerX1 = x1 = x2 = activeSize;
      y1 = edgeMargin;
      y2 = height - edgeMargin;
      activeArea->SetWidth(activeSize);
      this->Internal->LengthForRatio = height - edgeMargin - edgeMargin;
      break;
    case vtkAxis::BOTTOM:
      activeArea->SetY(height - activeSize);
      slicerY2 = height - sliceHandleMargin;
      slicerY1 = y1 = y2 = (height - activeSize);
      x1 = edgeMargin;
      x2 = width - edgeMargin;
      activeArea->SetHeight(activeSize);
      this->Internal->LengthForRatio = width - edgeMargin - edgeMargin;
      break;
    case vtkAxis::TOP:
      slicerY2 = sliceHandleMargin;
      slicerY1 = y1 = y2 = activeSize;
      x1 = edgeMargin;
      x2 = width - edgeMargin;
      activeArea->SetHeight(activeSize);
      this->Internal->LengthForRatio = width - edgeMargin - edgeMargin;
      break;
  }

  // Update the axis
  axis->SetPoint1(x1, y1);
  axis->SetPoint2(x2, y2);
  axis->GetBoundingRect(painter);
  axis->Update();

  // Draw the slices positions
  if (this->Internal->Slices.size() > 0)
  {
    // Fill the triangles
    vtkNew<vtkBrush> brush;

    // Draw the triangles
    double range[2];
    float polyX[4], polyY[4];
    axis->GetRange(range);
    double rangeLength = range[1] - range[0];
    std::vector<double>::iterator pos = this->Internal->Slices.begin();
    int index = 0;
    for (; pos != this->Internal->Slices.end(); pos++, index++)
    {
      if (this->Internal->ActiveSliceIndex == index)
      {
        brush->SetColor(255, 255, 255);
        painter->ApplyBrush(brush.GetPointer());
      }
      else if (this->Internal->SlicesVisibility[index])
      {
        brush->SetColor(0, 0, 0);
        painter->ApplyBrush(brush.GetPointer());
      }
      else
      {
        brush->SetColor(200, 200, 200);
        painter->ApplyBrush(brush.GetPointer());
      }
      float screen =
        edgeMargin + (this->Internal->LengthForRatio * (*pos - range[0]) / rangeLength);
      switch (axis->GetPosition())
      {
        case vtkAxis::LEFT:
        case vtkAxis::RIGHT:
          polyX[0] = polyX[3] = slicerX1;
          polyY[0] = polyY[3] = screen;
          polyX[1] = slicerX2;
          polyY[1] = screen - 4;
          polyX[2] = slicerX2;
          polyY[2] = screen + 4;
          break;
        case vtkAxis::BOTTOM:
        case vtkAxis::TOP:
          polyX[0] = polyX[3] = screen;
          polyY[0] = polyY[3] = slicerY1;
          polyX[1] = screen - 4;
          polyY[1] = slicerY2;
          polyX[2] = screen + 4;
          polyY[2] = slicerY2;
          break;
      }
      painter->DrawPolygon(polyX, polyY, 4);
    }
  }

  return this->vtkContextItem::Paint(painter);
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::Hit(const vtkContextMouseEvent& mouse)
{
  vtkVector2f position = mouse.GetPos();
  return this->Internal->IsInActiveArea(position.GetX(), position.GetY());
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  vtkVector2f position = mouse.GetPos();
  double value = 0;
  switch (this->Internal->Axis->GetPosition())
  {
    case vtkAxis::LEFT:
    case vtkAxis::RIGHT:
      value = this->ScreenToRange(position.GetY() - this->Internal->EdgeMargin);
      break;
    case vtkAxis::BOTTOM:
    case vtkAxis::TOP:
      value = this->ScreenToRange(position.GetX() - this->Internal->EdgeMargin);
      break;
  }
  // find corresponding slice index
  this->Internal->ActiveSliceIndex = this->Internal->FindSliceIndex(value, this->ComputeEpsilon());

  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  // Toggle visibility
  if (this->Internal->ActiveSliceIndex != -1 &&
    mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON &&
    mouse.GetModifiers() == vtkContextMouseEvent::NO_MODIFIER)
  {
    this->Internal->SlicesVisibility[Internal->ActiveSliceIndex] =
      !this->Internal->SlicesVisibility[Internal->ActiveSliceIndex];
    this->InvokeEvent(vtkMultiSliceContextItem::ModifySliceEvent);
  }

  // Notify any release action to the user
  if (this->Internal->ActiveSliceIndex != -1)
  {
    int data[3] = { mouse.GetButton(), mouse.GetModifiers(), this->Internal->ActiveSliceIndex };
    this->InvokeEvent(vtkCommand::EndInteractionEvent, data);
  }

  // Deselect active slice
  this->Internal->ActiveSliceIndex = -1;
  this->forceRender();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  double value = 0;
  int index = -1;
  switch (this->Internal->Axis->GetPosition())
  {
    case vtkAxis::LEFT:
    case vtkAxis::RIGHT:
      value = this->ScreenToRange(mouse.GetPos().GetY() - this->Internal->EdgeMargin);
      break;
    case vtkAxis::BOTTOM:
    case vtkAxis::TOP:
      value = this->ScreenToRange(mouse.GetPos().GetX() - this->Internal->EdgeMargin);
      break;
  }

  if ((index = this->Internal->FindSliceIndex(value, this->ComputeEpsilon())) == -1)
  {
    // Create
    index = static_cast<int>(this->Internal->Slices.size());
    this->Internal->Slices.push_back(value);
    this->Internal->SlicesVisibility.push_back(true);

    this->Internal->ActiveSliceIndex = index;
    this->InvokeEvent(vtkMultiSliceContextItem::AddSliceEvent);
    this->Internal->ActiveSliceIndex = -1;
  }
  else
  {
    // Delete
    std::vector<double>::iterator pos = this->Internal->Slices.begin();
    std::vector<bool>::iterator posV = this->Internal->SlicesVisibility.begin();
    pos += index;
    posV += index;
    this->Internal->Slices.erase(pos);
    this->Internal->SlicesVisibility.erase(posV);

    this->Internal->ActiveSliceIndex = index;
    this->InvokeEvent(vtkMultiSliceContextItem::RemoveSliceEvent);
    this->Internal->ActiveSliceIndex = -1;
  }

  // Make sure we paint that new slice
  this->forceRender();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (Internal->ActiveSliceIndex > -1)
  {
    double value = 0;
    switch (this->Internal->Axis->GetPosition())
    {
      case vtkAxis::LEFT:
      case vtkAxis::RIGHT:
        value = this->ScreenToRange(mouse.GetPos().GetY() - this->Internal->EdgeMargin);
        break;
      case vtkAxis::BOTTOM:
      case vtkAxis::TOP:
        value = this->ScreenToRange(mouse.GetPos().GetX() - this->Internal->EdgeMargin);
        break;
    }
    this->Internal->Slices[Internal->ActiveSliceIndex] = value;
    this->forceRender();
    this->InvokeEvent(vtkMultiSliceContextItem::ModifySliceEvent);
  }

  return true;
}

//-----------------------------------------------------------------------------
double vtkMultiSliceContextItem::ScreenToRange(float position)
{
  double range[2];
  this->Internal->Axis->GetRange(range);
  double rangeLength = range[1] - range[0];
  double value = range[0] + rangeLength * (position / this->Internal->LengthForRatio);

  // Cap the value to the range
  if (value > range[1])
  {
    value = range[1];
  }
  else if (value < range[0])
  {
    value = range[0];
  }

  return value;
}

//-----------------------------------------------------------------------------
double vtkMultiSliceContextItem::ComputeEpsilon(int numberOfPixel)
{
  double range[2];
  this->Internal->Axis->GetRange(range);
  double rangeLength = range[1] - range[0];
  return rangeLength * (numberOfPixel / this->Internal->LengthForRatio);
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::forceRender()
{
  // Mark the scene as dirty
  this->GetScene()->SetDirty(true);
}

//-----------------------------------------------------------------------------
const double* vtkMultiSliceContextItem::GetVisibleSlices(int& nbSlices) const
{
  return this->Internal->UpdateVisibleValues(nbSlices);
}

//-----------------------------------------------------------------------------
const double* vtkMultiSliceContextItem::GetSlices(int& nbSlices) const
{
  nbSlices = static_cast<int>(this->Internal->Slices.size());
  return nbSlices > 0 ? (&this->Internal->Slices[0]) : NULL;
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::SetSlices(double* values, bool* visibility, int numberOfSlices)
{
  this->Internal->Slices.clear();
  this->Internal->SlicesVisibility.clear();
  for (int i = 0; i < numberOfSlices; ++i)
  {
    this->Internal->Slices.push_back(values[i]);
    this->Internal->SlicesVisibility.push_back(visibility[i]);
  }
}

//-----------------------------------------------------------------------------
vtkAxis* vtkMultiSliceContextItem::GetAxis()
{
  return this->Internal->Axis.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::SetActiveSize(int size)
{
  this->Internal->ActiveSize = size;
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::SetEdgeMargin(int size)
{
  this->Internal->EdgeMargin = size;
}
//-----------------------------------------------------------------------------
double vtkMultiSliceContextItem::GetSliceValue(int sliceIndex)
{
  return this->Internal->Slices[sliceIndex];
}

//-----------------------------------------------------------------------------
int vtkMultiSliceContextItem::GetNumberOfSlices()
{
  return static_cast<int>(this->Internal->Slices.size());
}

//-----------------------------------------------------------------------------
int vtkMultiSliceContextItem::GetActiveSliceIndex()
{
  return this->Internal->ActiveSliceIndex;
}
