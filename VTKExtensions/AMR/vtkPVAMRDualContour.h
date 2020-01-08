/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAMRDualContour.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVAMRDualContour
 * @brief   Generates a contour surface given one or
 * more cell arrays and a volume fraction value.
 *
 *
 *
 * .SEE vtkAMRDualContour
 *
*/

#ifndef vtkPVAMRDualContour_h
#define vtkPVAMRDualContour_h

#include "vtkAMRDualContour.h"
#include "vtkPVVTKExtensionsAMRModule.h" //needed for exports

// Forware declaration.
class vtkPVAMRDualContourInternal;

class VTKPVVTKEXTENSIONSAMR_EXPORT vtkPVAMRDualContour : public vtkAMRDualContour
{
public:
  static vtkPVAMRDualContour* New();
  vtkTypeMacro(vtkPVAMRDualContour, vtkAMRDualContour);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkPVAMRDualContour();
  ~vtkPVAMRDualContour() override;

  //@{
  /**
   * Add to list of cell arrays which are used for generating contours.
   */
  void AddInputCellArrayToProcess(const char* name);
  void ClearInputCellArrayToProcess();
  //@}

  //@{
  /**
   * Get / Set volume fraction value.
   */
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);
  //@}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVAMRDualContour(const vtkPVAMRDualContour&) = delete;
  void operator=(const vtkPVAMRDualContour&) = delete;

protected:
  double VolumeFractionSurfaceValue;
  vtkPVAMRDualContourInternal* Implementation;
};

#endif // vtkPVAMRDualContour_h
