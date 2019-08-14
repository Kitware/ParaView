/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXYChartRepresentation
 *
 * vtkXYChartRepresentation is representation that is used to add vtkPlot
 * subclasses to a vtkChartXY instance e.g. adding vtkPlotBar to create a bar
 * chart or vtkPlotLine to create a line chart. For every selected series (or
 * column in a vtkTable), this class adds a new vtkPlot to the vtkChartXY.
 * vtkXYChartRepresentation provides a union of APIs for changing the appearance
 * of vtkPlot instances. Developers should only expose the applicable API in the
 * ServerManager XML.
 *
 * To select which type of vtkPlot instances this class will use, you must set
 * the ChartType. Refer to vtkChartXY::AddPlot() for details on what the type
 * must be.
*/

#ifndef vtkXYChartRepresentation_h
#define vtkXYChartRepresentation_h

#include "vtkChartRepresentation.h"

class vtkChartXY;
class vtkScalarsToColors;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkXYChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkXYChartRepresentation* New();
  vtkTypeMacro(vtkXYChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation. Overridden to ensure that internally
   * added vtkPlot instances are updated when hiding the representation.
   */
  void SetVisibility(bool visible) override;

  //@{
  /**
   * Get/Set the chart type, defaults to line chart. This must be set before
   * this representation is updated.
   * Valid values are vtkChart::LINE, vtkChart::POINTS, vtkChart::BAR, etc.
   * Default is vtkChart::LINE.
   */
  vtkSetMacro(ChartType, int);
  vtkGetMacro(ChartType, int);
  //@}

  void SetChartTypeToLine();
  void SetChartTypeToPoints();
  void SetChartTypeToBar();
  void SetChartTypeToStacked();
  void SetChartTypeToBag();
  void SetChartTypeToFunctionalBag();
  void SetChartTypeToArea();

  /**
   * Returns the vtkChartXY instance from the view to which this representation
   * is added. Thus this will return a non-null value only when this
   * representation is added to a view.
   */
  vtkChartXY* GetChart();

  //@{
  /**
   * Set the series to use as the X-axis.
   */
  vtkSetStringMacro(XAxisSeriesName);
  vtkGetStringMacro(XAxisSeriesName);
  //@}

  //@{
  /**
   * Set whether the index should be used for the x axis. When true, XSeriesName
   * is ignored.
   */
  vtkSetMacro(UseIndexForXAxis, bool);
  vtkGetMacro(UseIndexForXAxis, bool);
  //@}

  //@{
  /**
   * Get/set whether the points in the chart should be sorted by their x-axis value.
   * Points are connected in line plots in the order they are in the table.  Sorting
   * by the x-axis allows the line to have no cycles.
   */
  void SetSortDataByXAxis(bool val);
  vtkGetMacro(SortDataByXAxis, bool);
  //@}

  //@{
  /**
   * Set/Clear the properties for Y series/columns.
   */
  void SetSeriesVisibility(const char* seriesname, bool visible);
  void SetLineThickness(const char* name, double value);
  void SetLineStyle(const char* name, int value);
  void SetColor(const char* name, double r, double g, double b);
  void SetAxisCorner(const char* name, int corner);
  void SetMarkerStyle(const char* name, int style);
  void SetMarkerSize(const char* name, double value);
  void SetLabel(const char* name, const char* label);
  void SetUseColorMapping(const char* name, bool useColorMapping);
  void SetLookupTable(const char* name, vtkScalarsToColors* lut);
  const char* GetLabel(const char* name) const;
  //@}

  void ClearSeriesVisibilities();
  void ClearLineThicknesses();
  void ClearLineStyles();
  void ClearColors();
  void ClearAxisCorners();
  void ClearMarkerSizes();
  void ClearMarkerStyles();
  void ClearLabels();

  vtkSetVector3Macro(SelectionColor, double);
  vtkGetVector3Macro(SelectionColor, double);

  //@{
  /**
   * Get/Set the series label prefix.
   */
  vtkSetStringMacro(SeriesLabelPrefix);
  vtkGetStringMacro(SeriesLabelPrefix);
  //@}

  /**
   * Called by vtkPVContextView::Export() to export the representation's data to
   * a CSV file. Return false on failure which will call the exporting process
   * to abort and raise an error. Default implementation simply returns false.
   */
  bool Export(vtkCSVExporter* exporter) override;

protected:
  vtkXYChartRepresentation();
  ~vtkXYChartRepresentation() override;

  /**
   * Overridden to remove all plots from the view.
   */
  bool RemoveFromView(vtkView* view) override;

  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkSmartPointer<vtkDataObject> TransformTable(vtkSmartPointer<vtkDataObject>) override;

  void PrepareForRendering() override;

  class vtkInternals;
  friend class vtkInternals;
  vtkInternals* Internals;

  class SortTableFilter;

private:
  vtkXYChartRepresentation(const vtkXYChartRepresentation&) = delete;
  void operator=(const vtkXYChartRepresentation&) = delete;

  int ChartType;
  char* XAxisSeriesName;
  bool UseIndexForXAxis;
  bool SortDataByXAxis;
  bool PlotDataHasChanged;
  double SelectionColor[3];
  char* SeriesLabelPrefix;
};

#endif
