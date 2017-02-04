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
/**
 * @class   vtkPVParallelCoordinatesRepresentation
 *
 * vtkPVParallelCoordinatesRepresentation is the vtkChartParallelCoordinates
 * subclass for parallel coordinates representation. It exposes API from
 * underlying vtkChartParallelCoordinates.
*/

#ifndef vtkPVParallelCoordinatesRepresentation_h
#define vtkPVParallelCoordinatesRepresentation_h

#include "vtkChartRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkChartParallelCoordinates;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVParallelCoordinatesRepresentation
  : public vtkChartRepresentation
{
public:
  static vtkPVParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkPVParallelCoordinatesRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set visibility of the representation.
   */
  virtual void SetVisibility(bool visible) VTK_OVERRIDE;

  //@{
  /**
   * Set series visibility given its name. The order is currently ignored, but
   * in future we can add support to respect that as in
   * vtkPVPlotMatrixRepresentation.
   */
  void SetSeriesVisibility(const char* series, bool visibility);
  void ClearSeriesVisibilities();
  //@}

  /**
   * Provides access to the underlying VTK representation.
   */
  vtkChartParallelCoordinates* GetChart();

  //@{
  /**
   * Sets the line thickness for the plot.
   */
  vtkSetMacro(LineThickness, int);
  //@}

  //@{
  /**
   * Set the line style for the plot.
   */
  vtkSetMacro(LineStyle, int);
  //@}

  //@{
  /**
   * Sets the color to used for the lines in the plot.
   */
  vtkSetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Sets the opacity for the lines in the plot.
   */
  vtkSetMacro(Opacity, double);
  //@}

  /**
   * Called by vtkPVContextView::Export() to export the representation's data to
   * a CSV file. Return false on failure which will call the exporting process
   * to abort and raise an error. Default implementation simply returns false.
   */
  virtual bool Export(vtkCSVExporter* exporter) VTK_OVERRIDE;

protected:
  vtkPVParallelCoordinatesRepresentation();
  ~vtkPVParallelCoordinatesRepresentation();

  /**
   * Overridden to pass information about changes to series visibility etc. to
   * the plot-matrix.
   */
  virtual void PrepareForRendering() VTK_OVERRIDE;

  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  int LineThickness;
  int LineStyle;
  double Color[3];
  double Opacity;

private:
  vtkPVParallelCoordinatesRepresentation(
    const vtkPVParallelCoordinatesRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVParallelCoordinatesRepresentation&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
