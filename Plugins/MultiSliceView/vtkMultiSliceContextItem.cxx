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
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <vector>

// ******************************* Internal class *****************************
struct vtkMultiSliceContextItem::vtkInternal {
  std::vector<double> Slices;
  std::vector<bool> SlicesVisibility;
  double LengthForRatio;
  // We assume that we won't have more than 255 slices by axis
  double VisibleSliceValues[255];

  // ----

  // Return -1 if not found
  int FindSliceIndex(double value, double epsilon)
  {
    double min = value - epsilon;
    double max = value + epsilon;
    size_t size = this->Slices.size();
    for(size_t index=0; index < size; index++)
      {
      if(this->Slices[index] >= min && this->Slices[index] <= max)
        {
        return static_cast<int>(index);
        }
      }
    return -1;
  }

  // ----

  double* UpdateVisibleValues(int &size)
  {
    std::vector<bool>::iterator visIter = this->SlicesVisibility.begin();
    std::vector<double>::iterator valueIter = this->Slices.begin();
    for(size = 0; valueIter != this->Slices.end() && size < 255; valueIter++, visIter++)
      {
      if(*visIter)
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
  this->Axis = vtkAxis::New();
  this->AddItem(this->Axis);
  this->ActiveArea.Set(0,0,0,0);
  this->ActiveSliceIndex = -1; // Nothing is selected for now
  this->Internal = new vtkInternal();
}

//-----------------------------------------------------------------------------
vtkMultiSliceContextItem::~vtkMultiSliceContextItem()
{
  this->Axis->Delete();
  this->Axis = NULL;
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::Paint(vtkContext2D* painter)
{
  int width  = this->GetScene()->GetViewWidth();
  int height = this->GetScene()->GetViewHeight();
  int x1,y1,x2,y2;
  float slicerX1, slicerY1, slicerX2, slicerY2;
  slicerX2 = slicerY2 = 5;

  // Reset active area
  this->ActiveArea.Set(0, 0, width, height);

  switch(this->Axis->GetPosition())
    {
  case vtkAxis::LEFT:
    this->ActiveArea.SetX(width/2);
    slicerX2 = width - 5;
  case vtkAxis::RIGHT:
    slicerX1 = x1 = x2 = width/2;
    y1 = 15;
    y2 = height - 15;
    this->ActiveArea.SetWidth(width/2);
    this->Internal->LengthForRatio = height - 30;
    break;
  case vtkAxis::BOTTOM:
    this->ActiveArea.SetY(height/2);
    slicerY2 = height - 5;
  case vtkAxis::TOP:
    slicerY1 = y1 = y2 = height/2;
    x1 = 15;
    x2 = width - 15;
    this->ActiveArea.SetHeight(height/2);
    this->Internal->LengthForRatio = width - 30;
  break;
    }

  // Update the axis
  this->Axis->SetPoint1(x1,y1);
  this->Axis->SetPoint2(x2,y2);
  this->Axis->GetBoundingRect(painter);
  this->Axis->Update();

  // Draw the slices positions
  if(this->Internal->Slices.size() > 0)
    {
    // Fill the triangles
    vtkNew<vtkBrush> brush;

    // Draw the triangles
    double range[2];
    float polyX[4], polyY[4];
    this->Axis->GetRange(range);
    double rangeLength = range[1]-range[0];
    std::vector<double>::iterator pos = this->Internal->Slices.begin();
    int index = 0;
    for(;pos != this->Internal->Slices.end(); pos++, index++)
      {
      if(this->ActiveSliceIndex == index)
        {
        brush->SetColor(255,255,255);
        painter->ApplyBrush(brush.GetPointer());
        }
      else if(this->Internal->SlicesVisibility[index])
        {
        brush->SetColor(0,0,0);
        painter->ApplyBrush(brush.GetPointer());
        }
      else
        {
        brush->SetColor(200,200,200);
        painter->ApplyBrush(brush.GetPointer());
        }
      float screen = 15 + (this->Internal->LengthForRatio * (*pos - range[0]) / rangeLength);
      switch(this->Axis->GetPosition())
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
bool vtkMultiSliceContextItem::Hit(const vtkContextMouseEvent &mouse)
{
  vtkVector2f position = mouse.GetPos();
  return
      position.X() >= this->ActiveArea.X() &&
      position.Y() >= this->ActiveArea.Y() &&
      position.X() <= (this->ActiveArea.X() + this->ActiveArea.Width()) &&
      position.Y() <= (this->ActiveArea.Y() + this->ActiveArea.Height());
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  vtkVector2f position = mouse.GetPos();
  double value = 0;
  switch(this->Axis->GetPosition())
    {
  case vtkAxis::LEFT:
  case vtkAxis::RIGHT:
     value = this->ScreenToRange(position.Y() - 15);
    break;
  case vtkAxis::BOTTOM:
  case vtkAxis::TOP:
    value = this->ScreenToRange(position.X() - 15);
    break;
    }
  this->ActiveSliceIndex = this->Internal->FindSliceIndex(value, this->ComputeEpsilon());// find corresponding slice index

  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  // Toggle visibility
  if( this->ActiveSliceIndex != -1 &&
      mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    this->Internal->SlicesVisibility[this->ActiveSliceIndex] =
        !this->Internal->SlicesVisibility[this->ActiveSliceIndex];
    this->InvokeEvent(vtkCommand::ModifiedEvent);
    }

  // Deselect active slice
  this->ActiveSliceIndex = -1;
  this->forceRender();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseDoubleClickEvent(const vtkContextMouseEvent &mouse)
{
  double value = 0;
  int index = -1;
  switch(this->Axis->GetPosition())
    {
  case vtkAxis::LEFT:
  case vtkAxis::RIGHT:
    value = this->ScreenToRange(mouse.GetPos().Y()-15);
    break;
  case vtkAxis::BOTTOM:
  case vtkAxis::TOP:
    value = this->ScreenToRange(mouse.GetPos().X()-15);
  break;
    }

  if((index = this->Internal->FindSliceIndex(value, this->ComputeEpsilon())) == -1)
    {
    // Create
    this->Internal->Slices.push_back(value);
    this->Internal->SlicesVisibility.push_back(true);
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
    }


  // Make sure we paint that new slice
  this->forceRender();
  this->InvokeEvent(vtkCommand::ModifiedEvent);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMultiSliceContextItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if(this->ActiveSliceIndex > -1)
    {
    double value = 0;
    switch(this->Axis->GetPosition())
      {
    case vtkAxis::LEFT:
    case vtkAxis::RIGHT:
      value = this->ScreenToRange(mouse.GetPos().Y()-15);
      break;
    case vtkAxis::BOTTOM:
    case vtkAxis::TOP:
      value = this->ScreenToRange(mouse.GetPos().X()-15);
    break;
      }
    this->Internal->Slices[this->ActiveSliceIndex] = value;
    this->forceRender();
    this->InvokeEvent(vtkCommand::ModifiedEvent);
    }

   return true;
}

//-----------------------------------------------------------------------------
double vtkMultiSliceContextItem::ScreenToRange(float position)
{
  double range[2];
  this->Axis->GetRange(range);
  double rangeLength = range[1]-range[0];
  double value = range[0] + rangeLength*(position/this->Internal->LengthForRatio);

  // Cap the value to the range
  if(value > range[1])
    {
    value = range[1];
    }
  else if(value < range[0])
    {
    value = range[0];
    }

  return value;
}

//-----------------------------------------------------------------------------
double vtkMultiSliceContextItem::ComputeEpsilon(int numberOfPixel)
{
  double range[2];
  this->Axis->GetRange(range);
  double rangeLength = range[1]-range[0];
  return rangeLength*(numberOfPixel/this->Internal->LengthForRatio);
}

//-----------------------------------------------------------------------------
void vtkMultiSliceContextItem::forceRender()
{
  // Mark the scene as dirty
  this->GetScene()->SetDirty(true);
}

//-----------------------------------------------------------------------------
const double* vtkMultiSliceContextItem::GetVisibleSlices(int &nbSlices) const
{
  return this->Internal->UpdateVisibleValues(nbSlices);
}
