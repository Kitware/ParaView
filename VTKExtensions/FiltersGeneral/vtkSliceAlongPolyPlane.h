/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceAlongPolyPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSliceAlongPolyPlane
 * @brief   slice a dataset along a polyplane
 *
 *
 * vtkSliceAlongPolyPlane is a filter that slices its first input dataset
 * along the surface defined by sliding the poly line in the second input
 * dataset along a line parallel to the Z-axis.
 *
 * @sa
 * vtkCutter vtkPolyPlane
*/

#ifndef vtkSliceAlongPolyPlane_h
#define vtkSliceAlongPolyPlane_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkDataSet;
class vtkPolyData;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkSliceAlongPolyPlane : public vtkDataObjectAlgorithm
{
public:
  static vtkSliceAlongPolyPlane* New();
  vtkTypeMacro(vtkSliceAlongPolyPlane, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  vtkSetMacro(Tolerance, double) vtkGetMacro(Tolerance, double)
    //@}

    protected : vtkSliceAlongPolyPlane();
  ~vtkSliceAlongPolyPlane() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * The actual algorithm for slice a dataset along a polyline.
   */
  virtual bool Execute(vtkDataSet* inputDataset, vtkPolyData* lineDataSet, vtkPolyData* output);

  /**
   * Cleans up input polydata.
   */
  void CleanPolyLine(vtkPolyData* input, vtkPolyData* output);

private:
  vtkSliceAlongPolyPlane(const vtkSliceAlongPolyPlane&) = delete;
  void operator=(const vtkSliceAlongPolyPlane&) = delete;

  double Tolerance;
};

#endif
