/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotMatrixView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVPlotMatrixView.h"

#include "vtkObjectFactory.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVPlotMatrixView);

//----------------------------------------------------------------------------
vtkPVPlotMatrixView::vtkPVPlotMatrixView()
{
  this->PlotMatrix = vtkScatterPlotMatrix::New();
  this->PlotMatrix->AddObserver(
    vtkCommand::SelectionChangedEvent, this,
    &vtkPVPlotMatrixView::PlotMatrixSelectionCallback);

  this->ContextView->GetScene()->AddItem(this->PlotMatrix);
}

//----------------------------------------------------------------------------
vtkPVPlotMatrixView::~vtkPVPlotMatrixView()
{
  this->PlotMatrix->Delete();
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkPVPlotMatrixView::GetContextItem()
{
  return this->PlotMatrix;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::PlotMatrixSelectionCallback(vtkObject*,
  unsigned long event, void*)
{
  // forward the SelectionChangedEvent
  this->InvokeEvent(event);
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitle(const char* title)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleFont(const char* family, 
  int pointSize, bool bold, bool italic)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotTitleFont(family, pointSize, bold, italic);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleColor(
  double red, double green, double blue)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotTitleColor(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleAlignment(int alignment)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotTitleAlignment(alignment);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetGridVisibility(int plotType, bool visible)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetGridVisibility(plotType, visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetGutter(float x, float y)
{
  if (this->PlotMatrix)
    {
    vtkVector2f gutter(x, y);
    this->PlotMatrix->SetGutter(gutter);
    }
}
//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetBorders(int left, int bottom, int right, int top)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetBorders(left, bottom, right, top);
    }
}
//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetBackgroundColor(int plotType,
   double red, double green, double blue, double alpha)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetBackgroundColor(plotType, red, green, blue, alpha);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisColor(int plotType, double red, double green,
                                         double blue)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisColor(plotType, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetGridColor(int plotType, double red, double green,
                                         double blue)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetGridColor(plotType, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelVisibility(int plotType, bool visible)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisLabelVisibility(plotType, visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelFont(int plotType, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisLabelFont(
      plotType, family, pointSize, bold, italic);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelColor(
  int plotType, double red, double green, double blue)
{
  if(this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisLabelColor(plotType, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelNotation(int plotType, int notation)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisLabelNotation(plotType, notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelPrecision(int plotType, int precision)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetAxisLabelPrecision(plotType, precision);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetTooltipNotation(int plotType, int notation)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetTooltipNotation(plotType, notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetTooltipPrecision(int plotType, int precision)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetTooltipPrecision(plotType, precision);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->PlotMatrix)
    {
    this->PlotMatrix->PrintSelf(os, indent);
    }
}
