/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotMatrixView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPVPlotMatrixView_h
#define vtkPVPlotMatrixView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVContextView.h"

class vtkScatterPlotMatrix;

#define GENERATE_PLOT_TYPE_DECLARATION(name, type)                                                 \
  void SetScatterPlot##name(type value);                                                           \
  void SetHistogram##name(type value);                                                             \
  void SetActivePlot##name(type value);

#define GENERATE_PLOT_TYPE_DECLARATION2(name, type1, type2)                                        \
  void SetScatterPlot##name(type1 value1, type2 value2);                                           \
  void SetHistogram##name(type1 value1, type2 value2);                                             \
  void SetActivePlot##name(type1 value1, type2 value2);

#define GENERATE_PLOT_TYPE_DECLARATION3(name, type1, type2, type3)                                 \
  void SetScatterPlot##name(type1 value1, type2 value2, type3 value3);                             \
  void SetHistogram##name(type1 value1, type2 value2, type3 value3);                               \
  void SetActivePlot##name(type1 value1, type2 value2, type3 value3);

#define GENERATE_PLOT_TYPE_DECLARATION4(name, type1, type2, type3, type4)                          \
  void SetScatterPlot##name(type1 value1, type2 value2, type3 value3, type4 value4);               \
  void SetHistogram##name(type1 value1, type2 value2, type3 value3, type4 value4);                 \
  void SetActivePlot##name(type1 value1, type2 value2, type3 value3, type4 value4);

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVPlotMatrixView : public vtkPVContextView
{
public:
  static vtkPVPlotMatrixView* New();
  vtkTypeMacro(vtkPVPlotMatrixView, vtkPVContextView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkAbstractContextItem* GetContextItem() override;

  /**
   * Representations can use this method to set the selection for a particular
   * representation. Subclasses override this method to pass on the selection to
   * the chart using annotation link. Note this is meant to pass selection for
   * the local process alone. The view does not manage data movement for the
   * selection.
   */
  void SetSelection(vtkChartRepresentation* repr, vtkSelection* selection) override;

  //@{
  /**
   * Get/set the active plot in the scatter plot matrix.
   */
  void SetActivePlot(int i, int j);
  int GetActiveRow();
  int GetActiveColumn();
  //@}

  /**
   * Clear the animation path, ensuring it is empty.
   */
  void ClearAnimationPath();

  /**
   * Append to the animation path of the scatter plot matrix.
   */
  void AddAnimationPath(int i, int j);

  /**
   * Append to the animation path of the scatter plot matrix.
   */
  void StartAnimationPath();

  /**
   * Push the animation forward a frame.
   */
  void AdvanceAnimationPath();

  //@{
  /**
   * Get/Set the font of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleFont(const char* family, int pointSize, bool bold, bool italic) override;
  void SetTitleFontFamily(const char* family) override;
  void SetTitleFontSize(int pointSize) override;
  void SetTitleBold(bool bold) override;
  void SetTitleItalic(bool italic) override;
  void SetTitleFontFile(const char* file) override;
  const char* GetTitleFontFamily() override;
  int GetTitleFontSize() override;
  int GetTitleFontBold() override;
  int GetTitleFontItalic() override;
  //@}

  //@{
  /**
   * Get/Set the color of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleColor(double red, double green, double blue) override;
  double* GetTitleColor() override;
  //@}

  //@{
  /**
   * Set the alignement of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTitleAlignment(int alignment) override;
  int GetTitleAlignment() override;
  //@}

  /**
   * Set the number of animation frames used when changing the active scatterplot.
   */
  void SetNumberOfAnimationFrames(int value);

  //@{
  /**
   * Set the gutter that should be left between the charts in the matrix.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  virtual void SetGutter(float x, float y);
  void SetGutterX(float value);
  void SetGutterY(float value);
  //@}

  //@{
  /**
   * Set/get the borders of the chart matrix (space in pixels around each chart).
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  virtual void SetBorders(int left, int bottom, int right, int top);
  virtual void SetBorderLeft(int value);
  virtual void SetBorderBottom(int value);
  virtual void SetBorderRight(int value);
  virtual void SetBorderTop(int value);
  //@}

  //@{
  /**
   * Sets whether or not the grid for the given axis is visible given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetGridVisibility(int plotType, bool visible);
  GENERATE_PLOT_TYPE_DECLARATION(GridVisibility, bool);
  int GetGridVisibility(int plotType);
  //@}

  //@{
  /**
   * Sets the background color for the chart given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetBackgroundColor(int plotType, double red, double green, double blue, double alpha);
  double* GetBackgroundColor(int plotType);
  GENERATE_PLOT_TYPE_DECLARATION4(BackgroundColor, double, double, double, double);
  //@}

  //@{
  /**
   * Sets the color for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisColor(int plotType, double red, double green, double blue);
  double* GetAxisColor(int plotType);
  GENERATE_PLOT_TYPE_DECLARATION3(AxisColor, double, double, double);
  //@}

  //@{
  /**
   * Sets the color for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetGridColor(int plotType, double red, double green, double blue);
  double* GetGridColor(int plotType);
  GENERATE_PLOT_TYPE_DECLARATION3(GridColor, double, double, double);
  //@}

  //@{
  /**
   * Sets whether or not the labels for the axes are visible, given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelVisibility(int plotType, bool visible);
  int GetAxisLabelVisibility(int plotType);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelVisibility, bool);
  //@}

  //@{
  /**
   * Set the axis label font for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelFont(int plotType, const char* family, int pointSize, bool bold, bool italic);
  void SetAxisLabelFontFamily(int plotType, const char* family);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelFontFamily, const char*);
  void SetAxisLabelFontFile(int plotType, const char* file);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelFontFile, const char*);
  void SetAxisLabelFontSize(int plotType, int pointSize);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelFontSize, int);
  void SetAxisLabelBold(int plotType, bool bold);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelBold, bool);
  void SetAxisLabelItalic(int plotType, bool italic);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelItalic, bool);
  const char* GetAxisLabelFontFamily(int plotType);
  int GetAxisLabelFontSize(int plotType);
  int GetAxisLabelFontBold(int plotType);
  int GetAxisLabelFontItalic(int plotType);
  //@}

  //@{
  /**
   * Sets the axis label color for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelColor(int plotType, double red, double green, double blue);
  GENERATE_PLOT_TYPE_DECLARATION3(AxisLabelColor, double, double, double);
  double* GetAxisLabelColor(int plotType);
  //@}

  //@{
  /**
   * Sets the axis label notation for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelNotation(int plotType, int notation);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelNotation, int);
  int GetAxisLabelNotation(int plotType);
  //@}

  //@{
  /**
   * Sets the axis label precision for the axes given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetAxisLabelPrecision(int plotType, int precision);
  GENERATE_PLOT_TYPE_DECLARATION(AxisLabelPrecision, int);
  int GetAxisLabelPrecision(int plotType);
  //@}

  //@{
  /**
   * Set chart's tooltip notation and precision, given a plot type, which refers to
   * vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetTooltipNotation(int plotType, int notation);
  GENERATE_PLOT_TYPE_DECLARATION(TooltipNotation, int);
  void SetTooltipPrecision(int plotType, int precision);
  GENERATE_PLOT_TYPE_DECLARATION(TooltipPrecision, int);
  int GetTooltipNotation(int plotType);
  int GetTooltipPrecision(int plotType);
  //@}

  //@{
  /**
   * Set the scatter plot title's color.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetScatterPlotSelectedRowColumnColor(double red, double green, double blue, double alpha);
  double* GetScatterPlotSelectedRowColumnColor();
  //@}

  //@{
  /**
   * Set the scatter plot title's color.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  void SetScatterPlotSelectedActiveColor(double red, double green, double blue, double alpha);
  double* GetScatterPlotSelectedActiveColor();
  //@}

  /**
   * Update all the settings
   */
  void UpdateSettings();

protected:
  vtkPVPlotMatrixView();
  ~vtkPVPlotMatrixView() override;

  /**
   * Actual rendering implementation.
   */
  void Render(bool interactive) override;

  /**
   * The callback function when SelectionChangedEvent is invoked from
   * the Big chart in vtkScatterPlotMatrix.
   */
  void PlotMatrixSelectionCallback(vtkObject*, unsigned long, void*);

private:
  vtkPVPlotMatrixView(const vtkPVPlotMatrixView&) = delete;
  void operator=(const vtkPVPlotMatrixView&) = delete;

  vtkScatterPlotMatrix* PlotMatrix;
};

#endif
