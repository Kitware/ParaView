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
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkChartParallelCoordinates;

class VTKREMOTINGVIEWS_EXPORT vtkPVParallelCoordinatesRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkPVParallelCoordinatesRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

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
  bool Export(vtkCSVExporter* exporter) override;

protected:
  vtkPVParallelCoordinatesRepresentation();
  ~vtkPVParallelCoordinatesRepresentation() override;

  /**
   * Overridden to pass information about changes to series visibility etc. to
   * the plot-matrix.
   */
  void PrepareForRendering() override;

  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  int LineThickness;
  int LineStyle;
  double Color[3];
  double Opacity;

private:
  vtkPVParallelCoordinatesRepresentation(const vtkPVParallelCoordinatesRepresentation&) = delete;
  void operator=(const vtkPVParallelCoordinatesRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
