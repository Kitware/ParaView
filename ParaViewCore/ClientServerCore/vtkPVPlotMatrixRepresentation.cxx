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
    plotMatrix->SetColor(this->ScatterPlotColor[0],
                         this->ScatterPlotColor[1],
                         this->ScatterPlotColor[2]);
    plotMatrix->SetHistogramColor(this->HistogramColor[0],
                                  this->HistogramColor[1],
                                  this->HistogramColor[2]);
    plotMatrix->SetActivePlotColor(this->ActivePlotColor[0],
                                   this->ActivePlotColor[1],
                                   this->ActivePlotColor[2]);
    plotMatrix->SetMarkerStyle(this->ScatterPlotMarkerStyle);
    plotMatrix->SetActivePlotMarkerStyle(this->ActivePlotMarkerStyle);
    plotMatrix->SetMarkerSize(this->ScatterPlotMarkerSize);
    plotMatrix->SetActivePlotMarkerSize(this->ActivePlotMarkerSize);
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
    plotMatrix->SetInput(this->GetLocalOutput());
    if(vtkAnnotationLink* annLink = plotMatrix->GetActiveAnnotationLink())
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
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetSeriesVisibility(const char *name, bool visible)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetColumnVisibility(name, visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetSeriesLabel(const char *name, const char *label)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetColor(double r, double g, double b)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetColor(r, g, b);
    }

  this->ScatterPlotColor[0] = r;
  this->ScatterPlotColor[1] = g;
  this->ScatterPlotColor[2] = b;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotColor(double r, double g, double b)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetActivePlotColor(r, g, b);
    }

  this->ActivePlotColor[0] = r;
  this->ActivePlotColor[1] = g;
  this->ActivePlotColor[2] = b;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetHistogramColor(double r, double g, double b)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetHistogramColor(r, g, b);
    }

  this->HistogramColor[0] = r;
  this->HistogramColor[1] = g;
  this->HistogramColor[2] = b;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerStyle(int style)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetMarkerStyle(style);
    }

  this->ScatterPlotMarkerStyle = style;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerStyle(int style)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetActivePlotMarkerStyle(style);
    }

  this->ActivePlotMarkerStyle = style;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerSize(double size)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetMarkerSize(size);
    }

  this->ScatterPlotMarkerSize = size;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerSize(double size)
{
  if(vtkScatterPlotMatrix *plotMatrix = this->GetPlotMatrix())
    {
    plotMatrix->SetActivePlotMarkerSize(size);
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
