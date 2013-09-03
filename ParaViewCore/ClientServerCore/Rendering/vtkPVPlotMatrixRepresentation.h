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
// .NAME vtkPVPlotMatrixRepresentation - vtkChartRepresentation subclass for
// scatter-plot-matrix representation.
// .SECTION Description:
// vtkPVPlotMatrixRepresentation manages representations in a
// vtkScatterPlotMatrix view. It exposes API that affects how the matrix is
// rendered as well as API to control which of the columns in the input vtkTable
// are to be plotted in the matrix.
//
// vtkPVPlotMatrixRepresentation currently does not support multiblock of tables
// and only the first table is rendered.
#ifndef _vtkPVPlotMatrixRepresentation_h
#define _vtkPVPlotMatrixRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkChartRepresentation.h"
#include "vtkColor.h" // for ivars

class vtkScatterPlotMatrix;
class vtkStringArray;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVPlotMatrixRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVPlotMatrixRepresentation* New();
  vtkTypeMacro(vtkPVPlotMatrixRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Sets the visibility for the plot matrix representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Set series visibility given its name. The order in which
  // SetSeriesVisibility is called is used to determine the order for each of
  // the plots in the grid.
  void SetSeriesVisibility(const char* series, bool visibility);
  void ClearSeriesVisibilities();

  // Description:
  // Sets the color for the scatter plots in the plot matrix.
  void SetColor(double r, double g, double b);

  // Description:
  // Sets the color for the active plot.
  void SetActivePlotColor(double r, double g, double b);

  // Description:
  // Sets the color for the histograms.
  void SetHistogramColor(double r, double g, double b);

  // Description:
  // Sets the marker style for the scatter plots.
  void SetMarkerStyle(int style);

  // Description:
  // Sets the marker style for the active plot.
  void SetActivePlotMarkerStyle(int style);

  // Description:
  // Sets the marker size for the scatter plots.
  void SetMarkerSize(double size);

  // Description:
  // Sets the marker size for the active plots.
  void SetActivePlotMarkerSize(double size);

  // Description:
  // Returns the scatter plot matrix.
  vtkScatterPlotMatrix* GetPlotMatrix() const;

protected:
  vtkPVPlotMatrixRepresentation();
  ~vtkPVPlotMatrixRepresentation();

  // Description:
  // Overridden to pass information about changes to series visibility etc. to
  // the plot-matrix.
  virtual void PrepareForRendering();

  // Description:
  // Add the plot matrix representation to the view.
  virtual bool AddToView(vtkView *view);

  // Description:
  // Remove the plot matrix representation from the view.
  virtual bool RemoveFromView(vtkView *view);

private:
  vtkPVPlotMatrixRepresentation(const vtkPVPlotMatrixRepresentation&); // Not implemented
  void operator=(const vtkPVPlotMatrixRepresentation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

  vtkColor4ub ActivePlotColor;
  vtkColor4ub ScatterPlotColor;
  vtkColor4ub HistogramColor;
  int ScatterPlotMarkerStyle;
  int ActivePlotMarkerStyle;
  double ScatterPlotMarkerSize;
  double ActivePlotMarkerSize;
};

#endif
