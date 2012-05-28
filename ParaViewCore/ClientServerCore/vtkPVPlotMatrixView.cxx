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
#include "vtkRenderWindow.h"
#include "vtkTextProperty.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVPlotMatrixView);

//----------------------------------------------------------------------------
vtkPVPlotMatrixView::vtkPVPlotMatrixView()
{
  this->PlotMatrix = vtkScatterPlotMatrix::New();
  this->PlotMatrix->AddObserver(
    vtkCommand::SelectionChangedEvent, this,
    &vtkPVPlotMatrixView::PlotMatrixSelectionCallback);
  this->PlotMatrix->AddObserver(
    vtkCommand::AnnotationChangedEvent, this,
    &vtkPVPlotMatrixView::PlotMatrixSelectionCallback);
  this->PlotMatrix->AddObserver(
    vtkCommand::CreateTimerEvent, this,
    &vtkPVPlotMatrixView::PlotMatrixSelectionCallback);
  this->PlotMatrix->AddObserver(
    vtkCommand::AnimationCueTickEvent, this,
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
  // forward the SelectionChangedEvent and InteractionEvent
  this->InvokeEvent(event);
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetActivePlot(int i, int j)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetActivePlot(vtkVector2i(i, j));
    }
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetActiveRow()
{
  if (this->PlotMatrix)
    {
    return this->PlotMatrix->GetActivePlot().X();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetActiveColumn()
{
  if (this->PlotMatrix)
    {
    return this->PlotMatrix->GetActivePlot().Y();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::ClearAnimationPath()
{
  this->PlotMatrix->ClearAnimationPath();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::AddAnimationPath(int i, int j)
{
  this->PlotMatrix->AddAnimationPath(vtkVector2i(i, j));
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::StartAnimationPath()
{
  this->PlotMatrix->BeginAnimationPath(this->RenderWindow->GetInteractor());
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::AdvanceAnimationPath()
{
  this->PlotMatrix->AdvanceAnimation();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitle(const char* title)
{
  if (this->PlotMatrix)
    {
    this->PlotMatrix->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleFont(const char* family, 
  int pointSize, bool bold, bool italic)
{
  if (this->PlotMatrix)
    {
    vtkTextProperty *prop = this->PlotMatrix->GetTitleProperties();
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(bold);
    prop->SetItalic(italic);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleColor(
  double red, double green, double blue)
{
  if (this->PlotMatrix)
    {
    vtkTextProperty *prop = this->PlotMatrix->GetTitleProperties();
    prop->SetColor(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotTitleAlignment(int alignment)
{
  if (this->PlotMatrix)
    {
    vtkTextProperty *prop = this->PlotMatrix->GetTitleProperties();
    prop->SetJustification(alignment);
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
    vtkColor4ub color(static_cast<unsigned char>(red * 255),
                      static_cast<unsigned char>(green * 255),
                      static_cast<unsigned char>(blue * 255),
                      static_cast<unsigned char>(alpha * 255));
    this->PlotMatrix->SetBackgroundColor(plotType, color);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisColor(int plotType, double red, double green,
                                       double blue)
{
  if (this->PlotMatrix)
    {
    vtkColor4ub color(static_cast<unsigned char>(red * 255),
                      static_cast<unsigned char>(green * 255),
                      static_cast<unsigned char>(blue * 255),
                      255);
    this->PlotMatrix->SetAxisColor(plotType, color);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetGridColor(int plotType, double red, double green,
                                       double blue)
{
  if (this->PlotMatrix)
    {
    vtkColor4ub color(static_cast<unsigned char>(red * 255),
                      static_cast<unsigned char>(green * 255),
                      static_cast<unsigned char>(blue * 255),
                      255);
    this->PlotMatrix->SetGridColor(plotType, color);
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
    vtkTextProperty *prop = this->PlotMatrix->GetAxisLabelProperties(plotType);
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(bold);
    prop->SetItalic(italic);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetAxisLabelColor(
  int plotType, double red, double green, double blue)
{
  if(this->PlotMatrix)
    {
    vtkTextProperty *prop = this->PlotMatrix->GetAxisLabelProperties(plotType);
    prop->SetColor(red, green, blue);
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
    vtkColor4ub color(static_cast<unsigned char>(red * 255),
                      static_cast<unsigned char>(green * 255),
                      static_cast<unsigned char>(blue * 255),
                      static_cast<unsigned char>(alpha * 255));
    this->PlotMatrix->SetScatterPlotSelectedRowColumnColor(color);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::SetScatterPlotSelectedActiveColor(
  double red, double green, double blue, double alpha)
{
  if(this->PlotMatrix)
    {
    vtkColor4ub color(static_cast<unsigned char>(red * 255),
                      static_cast<unsigned char>(green * 255),
                      static_cast<unsigned char>(blue * 255),
                      static_cast<unsigned char>(alpha * 255));
    this->PlotMatrix->SetScatterPlotSelectedActiveColor(color);
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
    this->PlotMatrix->GetTitleProperties()->GetFontFamilyAsString() : NULL;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontSize()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTitleProperties()->GetFontSize() : -1;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontBold()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTitleProperties()->GetBold() : 0;
}
int vtkPVPlotMatrixView::GetScatterPlotTitleFontItalic()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTitleProperties()->GetItalic() : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetScatterPlotTitleColor()
{
  // FIXME: Do we need this style of API? It means keeping around a double[3]
  // in order to support it. I don't like it, and would rather avoid it.
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTitleProperties()->GetColor() : NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVPlotMatrixView::GetScatterPlotTitle()
{
  return this->PlotMatrix ? this->PlotMatrix->GetTitle() : NULL;
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixView::GetScatterPlotTitleAlignment()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetTitleProperties()->GetJustification() : 0;
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
    this->PlotMatrix->GetBackgroundColor(plotType).Cast<double>().GetData() : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetAxisColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisColor(plotType).Cast<double>().GetData() : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetGridColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetGridColor(plotType).Cast<double>().GetData() : NULL;
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
    this->PlotMatrix->GetAxisLabelProperties(plotType)->GetFontFamilyAsString()
      : NULL;
}
int vtkPVPlotMatrixView::GetAxisLabelFontSize(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelProperties(plotType)->GetFontSize() : -1;
}
int vtkPVPlotMatrixView::GetAxisLabelFontBold(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelProperties(plotType)->GetBold() : 0;
}
int vtkPVPlotMatrixView::GetAxisLabelFontItalic(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelProperties(plotType)->GetItalic() : 0;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetAxisLabelColor(int plotType)
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetAxisLabelProperties(plotType)->GetColor() : NULL;
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
    this->PlotMatrix->GetScatterPlotSelectedRowColumnColor().Cast<double>().GetData() : NULL;
}

//----------------------------------------------------------------------------
double* vtkPVPlotMatrixView::GetScatterPlotSelectedActiveColor()
{
  return this->PlotMatrix ? 
    this->PlotMatrix->GetScatterPlotSelectedActiveColor().Cast<double>().GetData() : NULL;
}
