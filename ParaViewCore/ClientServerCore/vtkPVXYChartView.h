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
// .NAME vtkPVXYChartView - view proxy for vtkQtBarChartView
// .SECTION Description
// vtkPVXYChartView is a concrete subclass of vtkSMChartViewProxy that
// creates a vtkQtBarChartView as the chart view.

#ifndef __vtkPVXYChartView_h
#define __vtkPVXYChartView_h

#include "vtkPVContextView.h"
#include "vtkAxis.h" //for enums.

class vtkChart;
class vtkChartView;
class vtkPVPlotTime;

class VTK_EXPORT vtkPVXYChartView : public vtkPVContextView
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
  // Sets whether or not the given axis is visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisVisibility(int index, bool visible);

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
  // For axis ranges, ParaView overrides the VTK charts behavior. Instead of
  // letting the user choose the behavior, we only have 2 modes, if any range is
  // set, then the range is always used. If no range is set, then alone we let
  // the chart determine the range.
  void SetLeftAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::LEFT, minimum, maximum); }
  void SetRightAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::RIGHT, minimum, maximum); }
  void SetTopAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::TOP, minimum, maximum); }
  void SetBottomAxisRange(double minimum, double maximum)
    { this->SetAxisRange(vtkAxis::BOTTOM, minimum, maximum); }
  void UnsetLeftAxisRange()
    { this->UnsetAxisRange(vtkAxis::LEFT); }
  void UnsetRightAxisRange()
    { this->UnsetAxisRange(vtkAxis::RIGHT); }
  void UnsetTopAxisRange()
    { this->UnsetAxisRange(vtkAxis::TOP); }
  void UnsetBottomAxisRange()
    { this->UnsetAxisRange(vtkAxis::BOTTOM); }

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

  void SelectionChanged();

  // Command implementation and object pointer - listen to selection updates.
  class CommandImpl;
  CommandImpl* Command;

private:
  vtkPVXYChartView(const vtkPVXYChartView&); // Not implemented
  void operator=(const vtkPVXYChartView&); // Not implemented
//ETX
};

#endif
