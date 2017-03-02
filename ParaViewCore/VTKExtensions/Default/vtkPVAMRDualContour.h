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
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

// Forware declaration.
class vtkPVAMRDualContourInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVAMRDualContour : public vtkAMRDualContour
{
public:
  static vtkPVAMRDualContour* New();
  vtkTypeMacro(vtkPVAMRDualContour, vtkAMRDualContour);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkPVAMRDualContour();
  virtual ~vtkPVAMRDualContour();

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

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkPVAMRDualContour(const vtkPVAMRDualContour&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVAMRDualContour&) VTK_DELETE_FUNCTION;

protected:
  double VolumeFractionSurfaceValue;
  vtkPVAMRDualContourInternal* Implementation;
};

#endif // vtkPVAMRDualContour_h
