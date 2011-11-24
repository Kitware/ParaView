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
void vtkPVPlotMatrixView::SetScatterPlotSelectedRowColumnColor(
  double red, double green, double blue, double alpha)
{
  if(this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotSelectedRowColumnColor(red, green, blue, alpha);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotSelectedActiveColor(
  double red, double green, double blue, double alpha)
{
  if(this->PlotMatrix)
    {
    this->PlotMatrix->SetScatterPlotSelectedActiveColor(red, green, blue, alpha);
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
//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::UpdateSettings()
{
  if(this->PlotMatrix)
    {
    this->PlotMatrix->UpdateSettings();
    }
}

//----------------------------------------------------------------------------
const char* vtkPVPlotMatrixView::GetScatterPlotTitleFontFamily()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleFontFamily() : NULL;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontSize()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleFontSize() : -1;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontBold()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleFontBold() : 0;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontItalic()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleFontItalic() : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetScatterPlotTitleColor()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleColor() : NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVPlotMatrixView::GetScatterPlotTitle()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitle() : NULL;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetScatterPlotTitleAlignment()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotTitleAlignment() : 0;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetGridVisibility(int plotType)
{
  return this->PlotMatrix ? 
    (this->PlotMatrix->GetGridVisibility(plotType)?1:0) : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetBackgroundColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetBackgroundColor(plotType) : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetAxisColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisColor(plotType) : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetGridColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetGridColor(plotType) : NULL;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetAxisLabelVisibility(int plotType)
{
  return this->PlotMatrix ? 
    (this->PlotMatrix->GetAxisLabelVisibility(plotType)?1:0) : 0;
}

//----------------------------------------------------------------------------
const char* vtkPVPlotMatrixView::GetAxisLabelFontFamily(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelFontFamily(plotType) : NULL;
}
int vtkPVPlotMatrixView::GetAxisLabelFontSize(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelFontSize(plotType) : -1;
}
int vtkPVPlotMatrixView::GetAxisLabelFontBold(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelFontBold(plotType) : 0;
}
int vtkPVPlotMatrixView::GetAxisLabelFontItalic(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelFontItalic(plotType) : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetAxisLabelColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelColor(plotType) : NULL;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetAxisLabelNotation(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelNotation(plotType) : 0;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetAxisLabelPrecision(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelPrecision(plotType) : 0;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetTooltipNotation(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTooltipNotation(plotType) : 0;
}
int vtkPVPlotMatrixView::GetTooltipPrecision(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTooltipPrecision(plotType) : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetScatterPlotSelectedRowColumnColor()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotSelectedRowColumnColor() : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetScatterPlotSelectedActiveColor()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotSelectedActiveColor() : NULL;
}
