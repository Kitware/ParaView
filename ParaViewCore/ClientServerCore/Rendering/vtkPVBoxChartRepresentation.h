/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBoxChartRepresentation
// .SECTION Description
// vtkPVBoxChartRepresentation is the vtkChartBox
// subclass for box plots representation. It exposes API from
// underlying vtkChartBox.

#ifndef __vtkPVBoxChartRepresentation_h
#define __vtkPVBoxChartRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkChartRepresentation.h"

class vtkChartBox;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVBoxChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVBoxChartRepresentation* New();
  vtkTypeMacro(vtkPVBoxChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Set/Clear the properties for series/columns.
  void SetSeriesVisibility(const char* series, bool visibility);
  void SetSeriesColor(const char* name, double r, double g, double b);

  void ClearSeriesVisibilities();
  void ClearSeriesColors();

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartBox* GetChart();

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
  // Set the visibility of the legend (plot labels)
  vtkSetMacro(Legend, bool);

//BTX
protected:
  vtkPVBoxChartRepresentation();
  ~vtkPVBoxChartRepresentation();

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
  bool Legend;
private:
  vtkPVBoxChartRepresentation(
      const vtkPVBoxChartRepresentation&); // Not implemented
  void operator=(const vtkPVBoxChartRepresentation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
