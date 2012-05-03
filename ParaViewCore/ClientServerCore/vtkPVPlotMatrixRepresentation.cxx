/*=========================================================================

   Program: ParaView
   Module:    pqPlotMatrixDisplayPanel.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVPlotMatrixRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkPVContextView.h"
#include "vtkTable.h"
#include "vtkStdString.h"
#include "vtkPlotPoints.h"
#include "vtkAnnotationLink.h"
#include "vtkSelectionDeliveryFilter.h"
#include "vtkStringArray.h"
#include "vtkChartNamedOptions.h"

vtkStandardNewMacro(vtkPVPlotMatrixRepresentation);

//----------------------------------------------------------------------------
vtkPVPlotMatrixRepresentation::vtkPVPlotMatrixRepresentation()
{
  // default colors are black (0, 0, 0)
  for(int i = 0; i < 3; i++)
    {
    this->ScatterPlotColor[i] = 0;
    this->ActivePlotColor[i] = 0;
    this->HistogramColor[i] = 0;
    }
  this->ScatterPlotColor[3] = 255;
  this->ActivePlotColor[3] = 255;
  this->HistogramColor[3] = 255;

  this->ScatterPlotMarkerStyle = vtkPlotPoints::CIRCLE;
  this->ActivePlotMarkerStyle = vtkPlotPoints::CIRCLE;
  this->ScatterPlotMarkerSize = 5.0;
  this->ActivePlotMarkerSize = 8.0;
}

//----------------------------------------------------------------------------
vtkPVPlotMatrixRepresentation::~vtkPVPlotMatrixRepresentation()
{
}

//----------------------------------------------------------------------------
bool vtkPVPlotMatrixRepresentation::AddToView(vtkView *view)
{
  if(!this->Superclass::AddToView(view))
    {
    return false;
    }

  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetInput(this->GetLocalOutput());
    plotMatrix->SetVisible(true);

    // set chart properties
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::SCATTERPLOT,
                             this->ScatterPlotColor);
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::HISTOGRAM,
                             this->HistogramColor);
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::ACTIVEPLOT,
                             this->ActivePlotColor);
    plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::SCATTERPLOT,
                                   this->ScatterPlotMarkerStyle);
    plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::ACTIVEPLOT,
                                   this->ActivePlotMarkerStyle);
    plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::SCATTERPLOT,
                                  this->ScatterPlotMarkerSize);
    plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::ACTIVEPLOT,
                                  this->ActivePlotMarkerSize);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVPlotMatrixRepresentation::RemoveFromView(vtkView* view)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetInput(0);
    plotMatrix->SetVisible(false);
    this->OrderedColumns->SetNumberOfTuples(0);
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkPVPlotMatrixRepresentation::RequestData(vtkInformation *request,
                                               vtkInformationVector **inputVector,
                                               vtkInformationVector *outputVector)
{
  if(!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    vtkTable* plotInput = this->GetLocalOutput();
    plotMatrix->SetInput(plotInput);
    vtkIdType numCols = plotInput->GetNumberOfColumns();
    if(numCols != this->OrderedColumns->GetNumberOfTuples())
      {
      this->OrderedColumns->SetNumberOfTuples(numCols);
      for (vtkIdType i = 0; i < numCols; ++i)
        {
        this->OrderedColumns->SetValue(i, plotInput->GetColumnName(i));
        }
      }
    if (this->Options)
      {
      this->Options->UpdatePlotOptions();
      }

    if(vtkAnnotationLink* annLink = plotMatrix->GetAnnotationLink())
      {
      vtkSelection* sel = vtkSelection::SafeDownCast(
        this->SelectionDeliveryFilter->GetOutputDataObject(0));
      annLink->SetCurrentSelection(sel);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::MoveInputTableColumn(int fromCol, int toCol)
{
  if(this->OrderedColumns->GetNumberOfTuples()==0 || !this->GetPlotMatrix())
    {
    return;
    }

  if(fromCol == toCol || fromCol == (toCol-1) || fromCol < 0 || toCol < 0)
    {
    return;
    }
  int numCols = this->OrderedColumns->GetNumberOfTuples();
  if( fromCol >= numCols || toCol > numCols)
    {
    return;
    }

  std::vector<vtkStdString> newOrderedCols;
  vtkStringArray* orderedCols = this->OrderedColumns.GetPointer();
  vtkIdType c;
  if(toCol == numCols)
    {
    for(c=0; c<numCols; c++)
      {
      if(c!=fromCol)
        {
        newOrderedCols.push_back(orderedCols->GetValue(c));
        }
      }
    // move the fromCol to the end
    newOrderedCols.push_back(orderedCols->GetValue(fromCol));
    }
  // insert the fromCol before toCol
  else if(fromCol < toCol)
    {
    // move Cols in the middle up
    for(c=0; c<fromCol; c++)
      {
      newOrderedCols.push_back(orderedCols->GetValue(c));
      }
    for(c=fromCol+1; c<numCols; c++)
      {
      if(c == toCol)
        {
        newOrderedCols.push_back(orderedCols->GetValue(fromCol));
        }
      newOrderedCols.push_back(orderedCols->GetValue(c));
      }
    }
  else
    {
    for(c=0; c<toCol; c++)
      {
      newOrderedCols.push_back(orderedCols->GetValue(c));
      }
    newOrderedCols.push_back(orderedCols->GetValue(fromCol));
    for(c=toCol; c<numCols; c++)
      {
      if(c != fromCol)
        {
        newOrderedCols.push_back(orderedCols->GetValue(c));
        }
      }
    }

  // repopulate the orderedCols
  vtkIdType visId=0;
  vtkNew<vtkStringArray> newVisCols;
  std::vector<vtkStdString>::iterator arrayIt;
  for(arrayIt=newOrderedCols.begin(); arrayIt!=newOrderedCols.end(); ++arrayIt)
    {
    orderedCols->SetValue(visId++, *arrayIt);
    if(this->GetPlotMatrix()->GetColumnVisibility(*arrayIt))
      {
      newVisCols->InsertNextValue(*arrayIt);
      }
    }
  this->GetPlotMatrix()->SetVisibleColumns(newVisCols.GetPointer());
}

//----------------------------------------------------------------------------
const char* vtkPVPlotMatrixRepresentation::GetSeriesName(int col)
{
  if(col>=0 && col<this->OrderedColumns->GetNumberOfTuples())
    {
    return this->OrderedColumns->GetValue(col);
    }

  return this->Superclass::GetSeriesName(col);
}
//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetColor(double r, double g, double b)
{
  this->ScatterPlotColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
                                       static_cast<unsigned char>(g * 255),
                                       static_cast<unsigned char>(b * 255));
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::SCATTERPLOT,
                             this->ScatterPlotColor);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotColor(double r, double g, double b)
{
  this->ActivePlotColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
                                      static_cast<unsigned char>(g * 255),
                                      static_cast<unsigned char>(b * 255));
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::ACTIVEPLOT,
                             this->ActivePlotColor);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetHistogramColor(double r, double g, double b)
{
  this->HistogramColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
                                     static_cast<unsigned char>(g * 255),
                                     static_cast<unsigned char>(b * 255));
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotColor(vtkScatterPlotMatrix::HISTOGRAM,
                             this->HistogramColor);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerStyle(int style)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::SCATTERPLOT, style);
    }

  this->ScatterPlotMarkerStyle = style;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerStyle(int style)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::ACTIVEPLOT, style);
    }

  this->ActivePlotMarkerStyle = style;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerSize(double size)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::SCATTERPLOT, size);
    }

  this->ScatterPlotMarkerSize = size;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerSize(double size)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::ACTIVEPLOT, size);
    }

  this->ActivePlotMarkerSize = size;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkScatterPlotMatrix* vtkPVPlotMatrixRepresentation::GetPlotMatrix() const
{
  if(this->ContextView)
    {
    return vtkScatterPlotMatrix::SafeDownCast(this->ContextView->GetContextItem());
    }

  return 0;
}
