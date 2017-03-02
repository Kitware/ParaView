/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIsoVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIsoVolume
 * @brief   This filter extract cells using lower / upper
 * threshold set and vtkPVClipDataSet filter.
 *
 *
 *
 * @sa
 * vtkThreshold vtkPVClipDataSet
*/

#ifndef vtkIsoVolume_h
#define vtkIsoVolume_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

// Forware declarations.
class vtkPVClipDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkIsoVolume : public vtkDataObjectAlgorithm
{
public:
  static vtkIsoVolume* New();
  vtkTypeMacro(vtkIsoVolume, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  //@}

protected:
  vtkIsoVolume();
  ~vtkIsoVolume();

  // Usual data generation methods.
  virtual int RequestData(
    vtkInformation* request, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * This filter produces a vtkMultiBlockDataSet when the input is a
   * vtkCompositeDataSet otherwise, it produces a vtkUnstructuredGrid.
   */
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  vtkDataObject* Clip(
    vtkDataObject* input, double value, const char* array_name, int fieldAssociation, bool invert);

  double LowerThreshold;
  double UpperThreshold;

private:
  vtkIsoVolume(const vtkIsoVolume&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIsoVolume&) VTK_DELETE_FUNCTION;
};

#endif // vtkIsoVolume_h
