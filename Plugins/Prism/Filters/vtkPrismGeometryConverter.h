// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPrismGeometryConverter
 * @brief   converts/scales geometry of datasets in a prism view
 *
 * vtkPrismGeometryConverter is a filter that converts/scales geometry of datasets in a prism view.
 * It takes into account the prism view' bounds, log scale and aspect ratio.
 */
#ifndef vtkPrismGeometryConverter_h
#define vtkPrismGeometryConverter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkPrismFiltersModule.h" // For export macro

class vtkArrayCalculator;

class VTKPRISMFILTERS_EXPORT vtkPrismGeometryConverter : public vtkPolyDataAlgorithm
{
public:
  static vtkPrismGeometryConverter* New();
  vtkTypeMacro(vtkPrismGeometryConverter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the Prism Bounds.
   */
  vtkSetVector6Macro(PrismBounds, double);
  vtkGetVector6Macro(PrismBounds, double);
  ///@}

  ///@{
  /**
   * Set/Get If X axis will be Log Scaled
   */
  vtkSetMacro(LogScaleX, bool);
  vtkBooleanMacro(LogScaleX, bool);
  vtkGetMacro(LogScaleX, bool);
  ///@}

  ///@{
  /**
   * Set/Get If Y axis will be Log Scaled
   */
  vtkSetMacro(LogScaleY, bool);
  vtkBooleanMacro(LogScaleY, bool);
  vtkGetMacro(LogScaleY, bool);
  ///@}

  ///@{
  /**
   * Set/Get If Z axis will be Log Scaled
   */
  vtkSetMacro(LogScaleZ, bool);
  vtkBooleanMacro(LogScaleZ, bool);
  vtkGetMacro(LogScaleZ, bool);
  ///@}

  ///@{
  /**
   * Set/Get the aspect ratio.
   */
  vtkSetVector3Macro(AspectRatio, double);
  vtkGetVector3Macro(AspectRatio, double);
  ///@}

protected:
  vtkPrismGeometryConverter();
  ~vtkPrismGeometryConverter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  double PrismBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  bool LogScaleX = false;
  bool LogScaleY = false;
  bool LogScaleZ = false;

  double AspectRatio[3] = { 1.0, 1.0, 1.0 };

  vtkNew<vtkArrayCalculator> Calculator;

private:
  vtkPrismGeometryConverter(const vtkPrismGeometryConverter&) = delete;
  void operator=(const vtkPrismGeometryConverter&) = delete;
};

#endif // vtkPrismGeometryConverter_h
