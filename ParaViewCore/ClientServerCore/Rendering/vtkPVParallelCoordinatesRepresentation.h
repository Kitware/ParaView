/*=========================================================================

  Program:   ParaView
  Module:    vtkPVParallelCoordinatesRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVParallelCoordinatesRepresentation
// .SECTION Description
// vtkPVParallelCoordinatesRepresentation is the vtkChartParallelCoordinates
// subclass for parallel coordinates representation. It exposes API from
// underlying vtkChartParallelCoordinates.

#ifndef __vtkPVParallelCoordinatesRepresentation_h
#define __vtkPVParallelCoordinatesRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkChartRepresentation.h"

class vtkChartParallelCoordinates;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVParallelCoordinatesRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkPVParallelCoordinatesRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Set series visibility given its name. The order is currently ignored, but
  // in future we can add support to respect that as in
  // vtkPVPlotMatrixRepresentation.
  void SetSeriesVisibility(const char* series, bool visibility);
  void ClearSeriesVisibilities();

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartParallelCoordinates* GetChart();

  // Description:
  // Sets the line thickness for the plot.
  vtkSetMacro(LineThickness, int);

  // Description:
  // Set the line style for the plot.
  vtkSetMacro(LineStyle, int);

  // Description:
  // Sets the color to used for the lines in the plot.
  vtkSetVector3Macro(Color, double);

  // Description:
  // Sets the opacity for the lines in the plot.
  vtkSetMacro(Opacity, double);

//BTX
protected:
  vtkPVParallelCoordinatesRepresentation();
  ~vtkPVParallelCoordinatesRepresentation();

  // Description:
  // Overridden to pass information about changes to series visibility etc. to
  // the plot-matrix.
  virtual void PrepareForRendering();

  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  int LineThickness;
  int LineStyle;
  double Color[3];
  double Opacity;
private:
  vtkPVParallelCoordinatesRepresentation(
      const vtkPVParallelCoordinatesRepresentation&); // Not implemented
  void operator=(const vtkPVParallelCoordinatesRepresentation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
