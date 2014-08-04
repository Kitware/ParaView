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
// .NAME vtkXYChartRepresentation
// .SECTION Description
// vtkXYChartRepresentation is representation that is used to add vtkPlot
// subclasses to a vtkChartXY instance e.g. adding vtkPlotBar to create a bar
// chart or vtkPlotLine to create a line chart. For every selected series (or
// column in a vtkTable), this class adds a new vtkPlot to the vtkChartXY.
// vtkXYChartRepresentation provides a union of APIs for changing the appearance
// of vtkPlot instances. Developers should only expose the applicable API in the
// ServerManager XML.
//
// To select which type of vtkPlot instances this class will use, you must set
// the ChartType. Refer to vtkChartXY::AddPlot() for details on what the type
// must be.

#ifndef __vtkXYChartRepresentation_h
#define __vtkXYChartRepresentation_h

#include "vtkChartRepresentation.h"

class vtkChartXY;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkXYChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkXYChartRepresentation* New();
  vtkTypeMacro(vtkXYChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set visibility of the representation. Overridden to ensure that internally
  // added vtkPlot instances are updated when hiding the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Get/Set the chart type, defaults to line chart. This must be set before
  // this representation is updated.
  // Valid values are vtkChart::LINE, vtkChart::POINTS, vtkChart::BAR, etc.
  // Default is vtkChart::LINE.
  vtkSetMacro(ChartType, int);
  vtkGetMacro(ChartType, int);
  void SetChartTypeToLine() {SetChartType(0);}
  void SetChartTypeToPoints() {SetChartType(1);}
  void SetChartTypeToBar() {SetChartType(2);}
  void SetChartTypeToStacked() {SetChartType(3);}
  void SetChartTypeToBag() {SetChartType(4);}
  void SetChartTypeToFunctionalBag() {SetChartType(5);}

  // Description:
  // Returns the vtkChartXY instance from the view to which this representation
  // is added. Thus this will return a non-null value only when this
  // representation is added to a view.
  vtkChartXY* GetChart();

  // Description:
  // Set the series to use as the X-axis.
  vtkSetStringMacro(XAxisSeriesName);
  vtkGetStringMacro(XAxisSeriesName);

  // Description:
  // Set whether the index should be used for the x axis. When true, XSeriesName
  // is ignored.
  vtkSetMacro(UseIndexForXAxis, bool);
  vtkGetMacro(UseIndexForXAxis, bool);

  // Description:
  // Set/Clear the properties for Y series/columns.
  void SetSeriesVisibility(const char* seriesname, bool visible);
  void SetLineThickness(const char* name, int value);
  void SetLineStyle(const char* name, int value);
  void SetColor(const char* name, double r, double g, double b);
  void SetAxisCorner(const char* name, int corner);
  void SetMarkerStyle(const char* name, int style);
  void SetLabel(const char* name, const char* label);
  const char* GetLabel(const char* name) const;

  void ClearSeriesVisibilities();
  void ClearLineThicknesses();
  void ClearLineStyles();
  void ClearColors();
  void ClearAxisCorners();
  void ClearMarkerStyles();
  void ClearLabels();

//BTX
protected:
  vtkXYChartRepresentation();
  ~vtkXYChartRepresentation();

  // Description:
  // Overridden to remove all plots from the view.
  virtual bool RemoveFromView(vtkView* view);

  virtual int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  virtual void PrepareForRendering();

private:
  vtkXYChartRepresentation(const vtkXYChartRepresentation&); // Not implemented
  void operator=(const vtkXYChartRepresentation&); // Not implemented

  class vtkInternals;
  friend class vtkInternals;
  vtkInternals* Internals;
  int ChartType;
  char* XAxisSeriesName;
  bool UseIndexForXAxis;
  bool PlotDataHasChanged;
//ETX
};

#endif
