/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractScatterPlot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractScatterPlot
 * @brief   Extract a scatter plot (two-dimensional histogram) from any dataset
 *
 *
 * vtkExtractScatterPlot accepts any vtkDataSet as input and produces a
 * vtkPolyData containing two-dimensional histogram data as output.  The
 * output vtkPolyData will contain two vtkDoubleArray instances named
 * "x_bin_extents" and "y_bin_extents", which contain the boundaries
 * between bins along each dimension.  It will also contain a
 * vtkUnsignedLongArray named "bin_values" which contains the value for
 * each bin.
*/

#ifndef vtkExtractScatterPlot_h
#define vtkExtractScatterPlot_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkExtractScatterPlot : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractScatterPlot* New();
  vtkTypeMacro(vtkExtractScatterPlot, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Controls which input data component should be binned, for input arrays with more-than-one
   * component
   */
  vtkSetClampMacro(XComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(XComponent, int);
  //@}

  //@{
  /**
   * Controls which input data component should be binned, for input arrays
   * with more-than-one component
   */
  vtkSetClampMacro(YComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(YComponent, int);
  //@}

  //@{
  /**
   * Controls the number of bins along the X axis in the output histogram data
   */
  vtkSetClampMacro(XBinCount, int, 1, VTK_INT_MAX);
  vtkGetMacro(XBinCount, int);
  //@}

  //@{
  /**
   * Controls the number of bins along the Y axis in the output histogram data
   */
  vtkSetClampMacro(YBinCount, int, 1, VTK_INT_MAX);
  vtkGetMacro(YBinCount, int);
  //@}

private:
  vtkExtractScatterPlot();
  vtkExtractScatterPlot(const vtkExtractScatterPlot&) = delete;
  void operator=(const vtkExtractScatterPlot&) = delete;
  ~vtkExtractScatterPlot() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int XComponent;
  int YComponent;
  int XBinCount;
  int YBinCount;
};

#endif
