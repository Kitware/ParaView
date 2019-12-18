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
/**
 * @class   vtkPVBoxChartRepresentation
 *
 * vtkPVBoxChartRepresentation is the vtkChartBox
 * subclass for box plots representation. It exposes API from
 * underlying vtkChartBox.
*/

#ifndef vtkPVBoxChartRepresentation_h
#define vtkPVBoxChartRepresentation_h

#include "vtkChartRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkChartBox;

class VTKREMOTINGVIEWS_EXPORT vtkPVBoxChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVBoxChartRepresentation* New();
  vtkTypeMacro(vtkPVBoxChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  //@{
  /**
   * Set/Clear the properties for series/columns.
   */
  void SetSeriesVisibility(const char* series, bool visibility);
  void SetSeriesColor(const char* name, double r, double g, double b);
  //@}

  void ClearSeriesVisibilities();
  void ClearSeriesColors();

  /**
   * Provides access to the underlying VTK representation.
   */
  vtkChartBox* GetChart();

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
   * Set the visibility of the legend (plot labels)
   */
  vtkSetMacro(Legend, bool);
  //@}

protected:
  vtkPVBoxChartRepresentation();
  ~vtkPVBoxChartRepresentation() override;

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
  bool Legend;

private:
  vtkPVBoxChartRepresentation(const vtkPVBoxChartRepresentation&) = delete;
  void operator=(const vtkPVBoxChartRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
