/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBagChartRepresentation
// .SECTION Description
// vtkPVagChartRepresentation is the vtkChartRepresentation
// subclass for bag plots representation. It exposes API from
// underlying vtkXYChart and vtkPlotBag.

#ifndef __vtkPVBagChartRepresentation_h
#define __vtkPVBagChartRepresentation_h

#include "vtkChartRepresentation.h"

class vtkChartXY;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVBagChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVBagChartRepresentation* New();
  vtkTypeMacro(vtkPVBagChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartXY* GetChart();

  // Description:
  // Set/get the line thickness for the plot.
  vtkSetMacro(LineThickness, int);
  vtkGetMacro(LineThickness, int);

  // Description:
  // Set/get the line style for the plot.
  vtkSetMacro(LineStyle, int);
  vtkGetMacro(LineStyle, int);

  // Description:
  // Set/get the color to used for the points in the plot.
  vtkSetVector3Macro(LineColor, double);
  vtkGetVector3Macro(LineColor, double);

  // Description:
  // Set/get the color to used for the bag in the plot.
  vtkSetVector3Macro(BagColor, double);
  vtkGetVector3Macro(BagColor, double);

  // Description:
  // Set/get the opacity for the bag in the plot.
  vtkSetMacro(Opacity, double);
  vtkGetMacro(Opacity, double);

  // Description:
  // Set/get the point size in the plot.
  vtkSetMacro(PointSize, int);
  vtkGetMacro(PointSize, int);

  // Description:
  // Set/get the color to used for the points in the plot.
  vtkSetVector3Macro(PointColor, double);
  vtkGetVector3Macro(PointColor, double);

  // Description:
  // Set/get the series to use as the X-axis.
  vtkSetStringMacro(XAxisSeriesName);
  vtkGetStringMacro(XAxisSeriesName);

  // Description:
  // Set/get whether the index should be used for the x axis. When true, XSeriesName
  // is ignored.
  vtkSetMacro(UseIndexForXAxis, bool);
  vtkGetMacro(UseIndexForXAxis, bool);

  // Description:
  // Set/get the series to use as the density
  vtkSetStringMacro(DensitySeriesName);
  vtkGetStringMacro(DensitySeriesName);

  // Description:
  // Set/get the series to use as the Y-axis
  vtkSetStringMacro(YAxisSeriesName);
  vtkGetStringMacro(YAxisSeriesName);

//BTX
protected:
  vtkPVBagChartRepresentation();
  ~vtkPVBagChartRepresentation();

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

private:
  vtkPVBagChartRepresentation(const vtkPVBagChartRepresentation&); // Not implemented
  void operator=(const vtkPVBagChartRepresentation&); // Not implemented

  int LineThickness;
  int LineStyle;
  double LineColor[3];
  double BagColor[3];
  double Opacity;
  int PointSize;
  double PointColor[3];
  char* XAxisSeriesName;
  char* YAxisSeriesName;
  char* DensitySeriesName;
  bool UseIndexForXAxis;
//ETX
};

#endif
