/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAMRDualClip.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAMRDualClip - Generates contour given one or more cell array
// and a volume fraction value.
//
// .SECTION Description
//
// .SEE vtkAMRDualClip
//

#ifndef vtkPVAMRDualClip_h
#define vtkPVAMRDualClip_h

#include "vtkAMRDualClip.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

// Forware declaration.
class vtkPVAMRDualClipInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVAMRDualClip : public vtkAMRDualClip
{
public:
  static vtkPVAMRDualClip* New();
  vtkTypeMacro(vtkPVAMRDualClip,vtkAMRDualClip);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkPVAMRDualClip();
  ~vtkPVAMRDualClip();

  // Description:
  // Add to list of cell arrays which are used for generating contours.
  void AddInputCellArrayToProcess(const char* name);
  void ClearInputCellArrayToProcess();

  // Description:
  // Get / Set volume fraction value.
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

private:
  vtkPVAMRDualClip(const vtkPVAMRDualClip&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVAMRDualClip&) VTK_DELETE_FUNCTION;

protected:

  double VolumeFractionSurfaceValue;

  vtkPVAMRDualClipInternal* Implementation;
};

#endif // vtkPVAMRDualClip_h
