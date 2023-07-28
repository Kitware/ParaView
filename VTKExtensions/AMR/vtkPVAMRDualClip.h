// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVAMRDualClip
 * @brief   Generates contour given one or more cell array
 * and a volume fraction value.
 *
 *
 *
 * .SEE vtkAMRDualClip
 *
 */

#ifndef vtkPVAMRDualClip_h
#define vtkPVAMRDualClip_h

#include "vtkAMRDualClip.h"
#include "vtkPVVTKExtensionsAMRModule.h" //needed for exports

// Forware declaration.
class vtkPVAMRDualClipInternal;

class VTKPVVTKEXTENSIONSAMR_EXPORT vtkPVAMRDualClip : public vtkAMRDualClip
{
public:
  static vtkPVAMRDualClip* New();
  vtkTypeMacro(vtkPVAMRDualClip, vtkAMRDualClip);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkPVAMRDualClip();
  ~vtkPVAMRDualClip() override;

  ///@{
  /**
   * Add to list of cell arrays which are used for generating contours.
   */
  void AddInputCellArrayToProcess(const char* name);
  void ClearInputCellArrayToProcess();
  ///@}

  ///@{
  /**
   * Get / Set volume fraction value.
   */
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);
  ///@}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVAMRDualClip(const vtkPVAMRDualClip&) = delete;
  void operator=(const vtkPVAMRDualClip&) = delete;

protected:
  double VolumeFractionSurfaceValue;

  vtkPVAMRDualClipInternal* Implementation;
};

#endif // vtkPVAMRDualClip_h
