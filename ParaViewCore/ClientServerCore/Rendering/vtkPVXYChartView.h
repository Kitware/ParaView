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
/**
 * @class   vtkPVXYChartView
 * @brief   vtkPVView subclass for drawing charts
 *
 * vtkPVXYChartView is a concrete subclass of vtkPVContextView -- which
 * in turn inherits vtkPVView -- that creates a vtkChart to perform
 * rendering.
*/

#ifndef vtkPVXYChartView_h
#define vtkPVXYChartView_h

#include "vtkAxis.h"                              //for enums.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVContextView.h"

class vtkChart;
class vtkPVPlotTime;
class vtkChartWarning;

#define GENERATE_AXIS_FUNCTIONS(name, type)                                                        \
  void SetLeft##name(type value) { Set##name(vtkAxis::LEFT, value); }                              \
  void SetBottom##name(type value) { Set##name(vtkAxis::BOTTOM, value); }                          \
  void SetRight##name(type value) { Set##name(vtkAxis::RIGHT, value); }                            \
  void SetTop##name(type value) { Set##name(vtkAxis::TOP, value); }

#define GENERATE_AXIS_FUNCTIONS2(name, type1, type2)                                               \
  void SetLeft##name(type1 value1, type2 value2) { Set##name(vtkAxis::LEFT, value1, value2); }     \
  void SetBottom##name(type1 value1, type2 value2) { Set##name(vtkAxis::BOTTOM, value1, value2); } \
  void SetRight##name(type1 value1, type2 value2) { Set##name(vtkAxis::RIGHT, value1, value2); }   \
  void SetTop##name(type1 value1, type2 value2) { Set##name(vtkAxis::TOP, value1, value2); }

#define GENERATE_AXIS_FUNCTIONS3(name, type1, type2, type3)                                        \
  void SetLeft##name(type1 value1, type2 value2, type3 value3)                                     \
  {                                                                                                \
    Set##name(vtkAxis::LEFT, value1, value2, value3);                                              \
  }                                                                                                \
  void SetBottom##name(type1 value1, type2 value2, type3 value3)                                   \
  {                                                                                                \
    Set##name(vtkAxis::BOTTOM, value1, value2, value3);                                            \
  }                                                                                                \
  void SetRight##name(type1 value1, type2 value2, type3 value3)                                    \
  {                                                                                                \
    Set##name(vtkAxis::RIGHT, value1, value2, value3);                                             \
  }                                                                                                \
  void SetTop##name(type1 value1, type2 value2, type3 value3)                                      \
  {                                                                                                \
    Set##name(vtkAxis::TOP, value1, value2, value3);                                               \
  }

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVXYChartView : public vtkPVContextView
{
public:
  static vtkPVXYChartView* New();
  vtkTypeMacro(vtkPVXYChartView, vtkPVContextView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the chart type, defaults to line chart
   */
  void SetChartType(const char* type);
  void SetChartTypeToLine() { this->SetChartType("Line"); }
  void SetChartTypeToBar() { this->SetChartType("Bar"); }
  void SetChartTypeToBag() { this->SetChartType("Bag"); }
  void SetChartTypeToBox() { this->SetChartType("Box"); }
  void SetChartTypeToArea() { this->SetChartType("Area"); }
  void SetChartTypeToFunctionalBag() { this->SetChartType("FunctionalBag"); }
  void SetChartTypeToParallelCoordinates() { this->SetChartType("ParallelCoordinates"); }

  /**
   * Set the title of the chart.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitle(const char* title);

  //@{
  /**
   * Set the chart title's font.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleFont(const char* family, int pointSize, bool bold, bool italic);
  void SetTitleFontFamily(const char* family);
  void SetTitleFontSize(int pointSize);
  void SetTitleBold(bool bold);
  void SetTitleItalic(bool bold);
  //@}

  /**
   * Set the chart title's color.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleColor(double red, double green, double blue);

  /**
   * Set the chart title's alignment.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleAlignment(int alignment);

  /**
   * Set the legend visibility.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetLegendVisibility(int visible);

  /**
   * Set the legend location.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetLegendLocation(int location);

  /**
   * Set the legend position.
   */
  void SetLegendPosition(int x, int y);

  /**
   * Set the legend font family.
   */
  void SetLegendFontFamily(const char* family);

  /**
   * Set the legend font size.
   */
  void SetLegendFontSize(int pointSize);

  /**
   * Set the legend font bold.
   */
  void SetLegendBold(bool bold);

  /**
   * Set the legend font italic.
   */
  void SetLegendItalic(bool italic);

  //@{
  /**
   * Sets whether or not the grid for the given axis is visible.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetGridVisibility(int index, bool visible);
  GENERATE_AXIS_FUNCTIONS(GridVisibility, bool);
  //@}

  //@{
  /**
   * Sets the color for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisColor(int index, double red, double green, double blue);
  GENERATE_AXIS_FUNCTIONS3(AxisColor, double, double, double);
  //@}

  //@{
  /**
   * Sets the color for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetGridColor(int index, double red, double green, double blue);
  GENERATE_AXIS_FUNCTIONS3(GridColor, double, double, double);
  //@}

  //@{
  /**
   * Sets whether or not the labels for the given axis are visible.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelVisibility(int index, bool visible);
  GENERATE_AXIS_FUNCTIONS(AxisLabelVisibility, bool);
  //@}

  /**
   * Set the axis label font for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelFont(int index, const char* family, int pointSize, bool bold, bool italic);

  //@{
  /**
   * Set the axis label font family for the given axis.
   */
  void SetAxisLabelFontFamily(int index, const char* family);
  GENERATE_AXIS_FUNCTIONS(AxisLabelFontFamily, const char*);
  //@}

  //@{
  /**
   * Set the axis label font size for the given axis.
   */
  void SetAxisLabelFontSize(int index, int pointSize);
  GENERATE_AXIS_FUNCTIONS(AxisLabelFontSize, int);
  //@}

  //@{
  /**
   * Set the axis label font bold for the given axis.
   */
  void SetAxisLabelBold(int index, bool bold);
  GENERATE_AXIS_FUNCTIONS(AxisLabelBold, bool);
  //@}

  //@{
  /**
   * Set the axis label font italic for the given axis.
   */
  void SetAxisLabelItalic(int index, bool italic);
  GENERATE_AXIS_FUNCTIONS(AxisLabelItalic, bool);
  //@}

  //@{
  /**
   * Sets the axis label color for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelColor(int index, double red, double green, double blue);
  GENERATE_AXIS_FUNCTIONS3(AxisLabelColor, double, double, double);
  //@}

  //@{
  /**
   * Sets the axis label notation for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelNotation(int index, int notation);
  GENERATE_AXIS_FUNCTIONS(AxisLabelNotation, int);
  //@}

  //@{
  /**
   * Sets the axis label precision for the given axis.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelPrecision(int index, int precision);
  GENERATE_AXIS_FUNCTIONS(AxisLabelPrecision, int);
  //@}

  //@{
  /**
   * For axis ranges, ParaView overrides the VTK charts behavior.
   * Users can either specify an explicit range or let the VTK chart determine
   * the range based on the data. To specify a range explicitly, users should
   * use SetAxisUseCustomRange() to on for the corresponding axis and then use
   * these methods to set the ranges. Note these ranges are only respected when
   * the corresponding AxisUseCustomRange flag it set.
   */
  GENERATE_AXIS_FUNCTIONS(AxisRangeMinimum, double);
  GENERATE_AXIS_FUNCTIONS(AxisRangeMaximum, double);
  //@}

  //@{
  /**
   * Set whether to use the range specified by SetAxisRange(..) (or variants) or
   * to let the chart determine the range automatically based on the data being
   * shown.
   */
  void SetAxisUseCustomRange(int index, bool useCustomRange);
  GENERATE_AXIS_FUNCTIONS(AxisUseCustomRange, bool);
  //@}

  //@{
  /**
   * Sets whether or not the given axis uses a log10 scale.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLogScale(int index, bool logScale);
  GENERATE_AXIS_FUNCTIONS(AxisLogScale, bool);
  //@}

  //@{
  /**
   * Set the chart axis title for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitle(int index, const char* title);
  GENERATE_AXIS_FUNCTIONS(AxisTitle, const char*);
  //@}

  /**
   * Set the chart axis title's font for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleFont(int index, const char* family, int pointSize, bool bold, bool italic);

  //@{
  /**
   * Set the chart axis title's font family for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleFontFamily(int index, const char* family);
  GENERATE_AXIS_FUNCTIONS(AxisTitleFontFamily, const char*);
  //@}

  //@{
  /**
   * Set the chart axis title's font size for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleFontSize(int index, int pointSize);
  GENERATE_AXIS_FUNCTIONS(AxisTitleFontSize, int);
  //@}

  //@{
  /**
   * Set the chart axis title's font bold for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleBold(int index, bool bold);
  GENERATE_AXIS_FUNCTIONS(AxisTitleBold, bool);
  //@}

  //@{
  /**
   * Set the chart axis title's font italic for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleItalic(int index, bool italic);
  GENERATE_AXIS_FUNCTIONS(AxisTitleItalic, bool);
  //@}

  //@{
  /**
   * Set the chart axis title's color for the given index.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisTitleColor(int index, double red, double green, double blue);
  GENERATE_AXIS_FUNCTIONS3(AxisTitleColor, double, double, double);
  //@}

  //@{
  /**
   * Set whether the chart uses custom labels or if the labels/ticks are placed
   * automatically.
   */
  void SetAxisUseCustomLabels(int index, bool useCustomLabels);
  GENERATE_AXIS_FUNCTIONS(AxisUseCustomLabels, bool);
  //@}

  //@{
  /**
   * Set the number of labels for the supplied axis.
   */
  void SetAxisLabelsNumber(int axis, int number);
  GENERATE_AXIS_FUNCTIONS(AxisLabelsNumber, int);
  //@}

  //@{
  /**
   * Set the axis label positions for the supplied axis at the given index.
   */
  void SetAxisLabels(int axis, int index, double value);
  GENERATE_AXIS_FUNCTIONS2(AxisLabels, int, double);
  //@}

  void SetTooltipNotation(int notation);
  void SetTooltipPrecision(int precision);

  //@{
  /**
   * Set the visibility for the time-marker in the view. Note, you cannot force
   * the time-marker to be shown. One can only hide it when the view would have
   * shown it otherwise.
   */
  vtkSetMacro(HideTimeMarker, bool);
  vtkGetMacro(HideTimeMarker, bool);
  //@}

  //@{
  /**
   * Set whether to sort the data in the chart by the x-axis array so that line
   * plot connectivity is from left to right rather than semi-randomly by index.
   */
  vtkGetMacro(SortByXAxis, bool);
  vtkSetMacro(SortByXAxis, bool);
  //@}

  /**
   * Provides access to the chart view.
   */
  virtual vtkChart* GetChart();

  /**
   * Get the context item.
   */
  virtual vtkAbstractContextItem* GetContextItem() VTK_OVERRIDE;

  /**
   * Representations can use this method to set the selection for a particular
   * representation. Subclasses override this method to pass on the selection to
   * the chart using annotation link. Note this is meant to pass selection for
   * the local process alone. The view does not manage data movement for the
   * selection.
   */
  virtual void SetSelection(vtkChartRepresentation* repr, vtkSelection* selection) VTK_OVERRIDE;

  /**
   * Overridden to rescale axes range on every update.
   */
  virtual void Update() VTK_OVERRIDE;

protected:
  vtkPVXYChartView();
  ~vtkPVXYChartView();

  void SetAxisRangeMinimum(int index, double min);
  void SetAxisRangeMaximum(int index, double max);

  /**
   * Actual rendering implementation.
   */
  virtual void Render(bool interactive) VTK_OVERRIDE;

  //@{
  /**
   * Set the internal title, for managing time replacement in the chart title.
   */
  vtkSetStringMacro(InternalTitle);
  //@}

  /**
   * Store the unreplaced chart title in the case where ${TIME} is used...
   */
  char* InternalTitle;

  //@{
  /**
   * Pointer to the proxy's chart instance.
   */
  vtkChart* Chart;
  vtkPVPlotTime* PlotTime;
  vtkChartWarning* LogScaleWarningLabel;
  //@}

  void SelectionChanged();

  bool HideTimeMarker;
  bool SortByXAxis;

private:
  vtkPVXYChartView(const vtkPVXYChartView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVXYChartView&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
