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

#ifndef __vtkPVAMRDualClip_h
#define __vtkPVAMRDualClip_h

#include "vtkAMRDualClip.h"

// Forware declaration.
class vtkPVAMRDualClipInternal;

class VTK_EXPORT vtkPVAMRDualClip : public vtkAMRDualClip
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

  //BTX
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

private:
  vtkPVAMRDualClip(const vtkPVAMRDualClip&);  // Not implemented.
  void operator=(const vtkPVAMRDualClip&);    // Not implemented.

  //ETX

protected:

  double VolumeFractionSurfaceValue;

  vtkPVAMRDualClipInternal* Implementation;
};

#endif // __vtkPVAMRDualClip_h
