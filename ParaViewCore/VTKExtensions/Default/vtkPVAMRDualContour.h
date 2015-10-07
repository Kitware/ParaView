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
// .NAME vtkPVAMRDualContour - Generates a contour surface given one or
// more cell arrays and a volume fraction value.
//
// .SECTION Description
//
// .SEE vtkAMRDualContour
//

#ifndef vtkPVAMRDualContour_h
#define vtkPVAMRDualContour_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkAMRDualContour.h"

// Forware declaration.
class vtkPVAMRDualContourInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVAMRDualContour : public vtkAMRDualContour
{
public:
  static vtkPVAMRDualContour* New();
  vtkTypeMacro(vtkPVAMRDualContour,vtkAMRDualContour);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkPVAMRDualContour();
  virtual ~vtkPVAMRDualContour();

  // Description:
  // Add to list of cell arrays which are used for generating contours.
  void AddInputCellArrayToProcess(const char* name);
  void ClearInputCellArrayToProcess();

  // Description:
  // Get / Set volume fraction value.
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);

  //BTX
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

private:
  vtkPVAMRDualContour(const vtkPVAMRDualContour&);  // Not implemented.
  void operator=(const vtkPVAMRDualContour&);    // Not implemented.

  //ETX

protected:
  double VolumeFractionSurfaceValue;
  vtkPVAMRDualContourInternal* Implementation;
};

#endif // vtkPVAMRDualContour_h
