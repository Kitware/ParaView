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
/**
 * @class   vtkPVBagChartRepresentation
 *
 * vtkPVagChartRepresentation is the vtkChartRepresentation
 * subclass for bag plots representation. It exposes API from
 * underlying vtkXYChart and vtkPlotBag.
*/

#ifndef vtkPVBagChartRepresentation_h
#define vtkPVBagChartRepresentation_h

#include "vtkBagPlotViewsAndFiltersBagPlotModule.h"
#include "vtkChartRepresentation.h"

class vtkChartXY;
class vtkImageData;
class vtkScalarsToColors;

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkPVBagChartRepresentation
  : public vtkChartRepresentation
{
public:
  static vtkPVBagChartRepresentation* New();
  vtkTypeMacro(vtkPVBagChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  /**
   * Provides access to the underlying VTK representation.
   */
  vtkChartXY* GetChart();

  //@{
  /**
   * Set/get the line thickness for the plot.
   */
  vtkSetMacro(LineThickness, int);
  vtkGetMacro(LineThickness, int);
  //@}

  //@{
  /**
   * Set/get the line style for the plot.
   */
  vtkSetMacro(LineStyle, int);
  vtkGetMacro(LineStyle, int);
  //@}

  //@{
  /**
   * Set/get the color to used for the points in the plot.
   */
  vtkSetVector3Macro(LineColor, double);
  vtkGetVector3Macro(LineColor, double);
  //@}

  //@{
  /**
   * Set/get the color to used for the points in the plot.
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  //@}

  //@{
  /**
   * Set/get the color to used for the bag in the plot.
   */
  vtkSetVector3Macro(BagColor, double);
  vtkGetVector3Macro(BagColor, double);
  //@}

  //@{
  /**
   * Set/get the color to used for the bag in the plot.
   */
  vtkSetVector3Macro(SelectionColor, double);
  vtkGetVector3Macro(SelectionColor, double);
  //@}

  //@{
  /**
   * Set/get the opacity for the bag in the plot.
   */
  vtkSetMacro(Opacity, double);
  vtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set/get the point size in the plot.
   */
  vtkSetMacro(PointSize, int);
  vtkGetMacro(PointSize, int);
  //@}

  //@{
  /**
   * Set/get the color to used for the points in the plot.
   */
  vtkSetVector3Macro(PointColor, double);
  vtkGetVector3Macro(PointColor, double);
  //@}

  //@{
  /**
   * Set/get the line thickness for the plot.
   */
  vtkSetMacro(GridLineThickness, int);
  vtkGetMacro(GridLineThickness, int);
  //@}

  //@{
  /**
   * Set/get the line style for the plot.
   */
  vtkSetMacro(GridLineStyle, int);
  vtkGetMacro(GridLineStyle, int);
  //@}

  //@{
  /**
   * Set/get the color to used for the user defined quartile isoline in the plot.
   */
  vtkSetVector3Macro(PUserColor, double);
  vtkGetVector3Macro(PUserColor, double);
  //@}

  //@{
  /**
   * Set/get the color to used for the P50 isoline in the plot.
   */
  vtkSetVector3Macro(P50Color, double);
  vtkGetVector3Macro(P50Color, double);
  //@}

  //@{
  /**
   * Set/get the series to use as the X-axis.
   */
  vtkSetStringMacro(XAxisSeriesName);
  vtkGetStringMacro(XAxisSeriesName);
  //@}

  //@{
  /**
   * Set/get whether the index should be used for the x axis. When true, XSeriesName
   * is ignored.
   */
  vtkSetMacro(UseIndexForXAxis, bool);
  vtkGetMacro(UseIndexForXAxis, bool);
  //@}

  //@{
  /**
   * Set/get the series to use as the density
   */
  vtkSetStringMacro(DensitySeriesName);
  vtkGetStringMacro(DensitySeriesName);
  //@}

  //@{
  /**
   * Set/get the series to use as the Y-axis
   */
  vtkSetStringMacro(YAxisSeriesName);
  vtkGetStringMacro(YAxisSeriesName);
  //@}

protected:
  vtkPVBagChartRepresentation();
  ~vtkPVBagChartRepresentation() override;

  /**
   * Overridden to pass information about changes to series visibility etc. to
   * the plot-matrix.
   */
  void PrepareForRendering() override;

  void SetPolyLineToTable(vtkPolyData* polydata, vtkTable* table);
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkPVBagChartRepresentation(const vtkPVBagChartRepresentation&) = delete;
  void operator=(const vtkPVBagChartRepresentation&) = delete;

  int LineThickness;
  int LineStyle;
  double LineColor[3];
  vtkScalarsToColors* LookupTable;
  double BagColor[3];
  double SelectionColor[3];
  double Opacity;
  int PointSize;
  double PointColor[3];
  int GridLineThickness;
  int GridLineStyle;
  double PUserColor[3];
  double P50Color[3];
  char* XAxisSeriesName;
  char* YAxisSeriesName;
  char* DensitySeriesName;
  bool UseIndexForXAxis;
  vtkSmartPointer<vtkImageData> LocalGrid;
  vtkSmartPointer<vtkTable> LocalThreshold;
};

#endif
