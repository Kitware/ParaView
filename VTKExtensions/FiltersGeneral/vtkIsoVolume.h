// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

// Forware declarations.
class vtkPVClipDataSet;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkIsoVolume : public vtkDataObjectAlgorithm
{
public:
  static vtkIsoVolume* New();
  vtkTypeMacro(vtkIsoVolume, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  ///@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  ///@}

protected:
  vtkIsoVolume();
  ~vtkIsoVolume() override;

  // Usual data generation methods.
  int RequestData(vtkInformation* request, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * This filter produces a vtkMultiBlockDataSet when the input is a
   * vtkCompositeDataSet otherwise, it produces a vtkUnstructuredGrid.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkDataObject* Clip(
    vtkDataObject* input, double value, const char* array_name, int fieldAssociation, bool invert);

  double LowerThreshold;
  double UpperThreshold;

private:
  vtkIsoVolume(const vtkIsoVolume&) = delete;
  void operator=(const vtkIsoVolume&) = delete;
};

#endif // vtkIsoVolume_h
