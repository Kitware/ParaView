/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXYChartView - vtkPVView subclass for drawing charts
// .SECTION Description
// vtkPVXYChartView is a concrete subclass of vtkPVContextView -- which
// in turn inherits vtkPVView -- that creates a vtkChart to perform
// rendering.

#ifndef __vtkPVXYChartView_h
#define __vtkPVXYChartView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVContextView.h"
#include "vtkAxis.h" //for enums.

class vtkChart;
class vtkPVPlotTime;
class vtkChartWarning;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVXYChartView : public vtkPVContextView
{
public:
  static vtkPVXYChartView* New();
  vtkTypeMacro(vtkPVXYChartView, vtkPVContextView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the chart type, defaults to line chart
  void SetChartType(const char *type);

  // Description:
  // Set the title of the chart.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitle(const char* title);

  // Description:
  // Set the chart title's font.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleFont(const char* family, int pointSize, bool bold, bool italic);

  // Description:
  // Set the chart title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleColor(double red, double green, double blue);

  // Description:
  // Set the chart title's alignment.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleAlignment(int alignment);

  // Description:
  // Set the legend visibility.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetLegendVisibility(int visible);

  // Description:
  // Set the legend location.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetLegendLocation(int location);

  // Description:
  // Sets whether or not the grid for the given axis is visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridVisibility(int index, bool visible);

  // Description:
  // Sets the color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisColor(int index, double red, double green, double blue);

  // Description:
  // Sets the color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridColor(int index, double red, double green, double blue);

  // Description:
  // Sets whether or not the labels for the given axis are visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelVisibility(int index, bool visible);

  // Description:
  // Set the axis label font for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelFont(int index, const char* family, int pointSize, bool bold,
                        bool italic);

  // Description:
  // Sets the axis label color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelColor(int index, double red, double green, double blue);

  // Description:
  // Sets the axis label notation for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelNotation(int index, int notation);

  // Description:
  // Sets the axis label precision for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelPrecision(int index, int precision);

  // Description:
  // For axis ranges, ParaView overrides the VTK charts behavior.
  // Users can either specify an explicit range or let the VTK chart determine
  // the range based on the data. To specify a range explicitly, users should
  // use SetAxisUseCustomRange() to on for the corresponding axis and then use
  // these methods to set the ranges. Note these ranges are only respected when
  // the corresponding AxisUseCustomRange flag it set.
  void SetLeftAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::LEFT, minimum, maximum); }
  void SetRightAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::RIGHT, minimum, maximum); }
  void SetTopAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::TOP, minimum, maximum); }
  void SetBottomAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::BOTTOM, minimum, maximum); }

  // Description:
  // Set whether to use the range specified by SetAxisRange(..) (or variants) or
  // to let the chart determine the range automatically based on the data being
  // shown.
  void SetAxisUseCustomRange(int index, bool useCustomRange);

  // Description:
  // Sets whether or not the given axis uses a log10 scale.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLogScale(int index, bool logScale);

  // Description:
  // Set the chart axis title for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitle(int index, const char* title);

  // Description:
  // Set the chart axis title's font for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitleFont(int index, const char* family, int pointSize,
                        bool bold, bool italic);

  // Description:
  // Set the chart axis title's color for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitleColor(int index, double red, double green, double blue);


  // Description:
  // Set whether the chart uses custom labels or if the labels/ticks are placed
  // automatically.
  void SetAxisUseCustomLabels(int index, bool use_custom_labels);

  // Description:
  // Set the number of labels for the supplied axis.
  void SetAxisLabelsNumber(int axis, int number);

  // Description:
  // Set the axis label positions for the supplied axis at the given index.
  void SetAxisLabels(int axis, int index, double value);

  // Description:
  // Set the number of axis labels for the left axis.
  void SetAxisLabelsLeftNumber(int number);

  // Description:
  // Set the label positions for the left axis.
  void SetAxisLabelsLeft(int index, double value);

  // Description:
  // Set the number of labels for the bottom axis.
  void SetAxisLabelsBottomNumber(int number);

  // Description:
  // Set the label positions for the bottom axis.
  void SetAxisLabelsBottom(int index, double value);

  // Description:
  // Set the number of labels for the right axis.
  void SetAxisLabelsRightNumber(int number);

  // Description:
  // Set the label positions for the right axis.
  void SetAxisLabelsRight(int index, double value);

  // Description:
  // Set the number of labels for the top axis.
  void SetAxisLabelsTopNumber(int number);

  // Description:
  // Set the label positions for the top axis.
  void SetAxisLabelsTop(int index, double value);

  void SetTooltipNotation(int notation);
  void SetTooltipPrecision(int precision);

  // Description:
  // Provides access to the chart view.
  virtual vtkChart* GetChart();

  // Description:
  // Get the context item.
  virtual vtkAbstractContextItem* GetContextItem();

  // Description:
  // Representations can use this method to set the selection for a particular
  // representation. Subclasses override this method to pass on the selection to
  // the chart using annotation link. Note this is meant to pass selection for
  // the local process alone. The view does not manage data movement for the
  // selection.
  virtual void SetSelection(
    vtkChartRepresentation* repr, vtkSelection* selection);

//BTX
protected:
  vtkPVXYChartView();
  ~vtkPVXYChartView();

  void SetAxisRange(int index, double min, double max);
  void UnsetAxisRange(int index);

  // Description:
  // Actual rendering implementation.
  virtual void Render(bool interactive);

  // Description:
  // Set the internal title, for managing time replacement in the chart title.
  vtkSetStringMacro(InternalTitle);

  // Description:
  // Store the unreplaced chart title in the case where ${TIME} is used...
  char* InternalTitle;

  // Description:
  // Pointer to the proxy's chart instance.
  vtkChart* Chart;
  vtkPVPlotTime* PlotTime;
  vtkChartWarning* LogScaleWarningLabel;

  void SelectionChanged();

private:
  vtkPVXYChartView(const vtkPVXYChartView&); // Not implemented
  void operator=(const vtkPVXYChartView&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
