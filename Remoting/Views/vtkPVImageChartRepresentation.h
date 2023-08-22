// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVImageChartRepresentation
 * @brief   Representation for the "Image Chart View".
 *
 * vtkPVImageChartRepresentation is the vtkChartRepresentation
 * subclass for image data representation, in a vtkPlotHistogram2D.
 *
 * If the input if a multi-block, it will display the first image
 * data it finds and skip the rest.
 */

#ifndef vtkPVImageChartRepresentation_h
#define vtkPVImageChartRepresentation_h

#include "vtkChartRepresentation.h"
#include "vtkSmartPointer.h"

class vtkChartHistogram2D;
class vtkScalarsToColors;
class vtkView;

class VTKREMOTINGVIEWS_EXPORT vtkPVImageChartRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVImageChartRepresentation* New();
  vtkTypeMacro(vtkPVImageChartRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  /**
   * Provides access to the underlying VTK representation.
   */
  vtkChartHistogram2D* GetChart();

  /**
   * Set the color map to use for the points in the plot.
   */
  void SetLookupTable(vtkScalarsToColors* lut);

protected:
  vtkPVImageChartRepresentation() = default;
  ~vtkPVImageChartRepresentation() override;

  void PrepareForRendering() override;
  bool RemoveFromView(vtkView* view) override;

  /**
   * Override to simply pass the data instead of converting it to vtkTable.
   * It is needed because the vtkChartHistogram2D takes a vtkImageData directly.
   */
  vtkSmartPointer<vtkDataObject> ReduceDataToRoot(vtkDataObject* data) override;

private:
  vtkPVImageChartRepresentation(const vtkPVImageChartRepresentation&) = delete;
  void operator=(const vtkPVImageChartRepresentation&) = delete;

  vtkSmartPointer<vtkScalarsToColors> LookupTable;
};

#endif
