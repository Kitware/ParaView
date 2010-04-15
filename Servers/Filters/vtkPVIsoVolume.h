/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIsoVolume.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIsoVolume - Generates contour given one or more cell array
// and a volume fraction value.
//
// .SECTION Description
//
// .SEE vtkAMRDualClip
//

#ifndef __vtkPVIsoVolume_h
#define __vtkPVIsoVolume_h

#include "vtkAMRDualClip.h"

// Forware declaration.
class vtkPVIsoVolumeInternal;

class VTK_EXPORT vtkPVIsoVolume : public vtkAMRDualClip
{
public:
  static vtkPVIsoVolume* New();
  vtkTypeRevisionMacro(vtkPVIsoVolume,vtkAMRDualClip);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkPVIsoVolume();
  ~vtkPVIsoVolume();

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
  vtkPVIsoVolume(const vtkPVIsoVolume&);  // Not implemented.
  void operator=(const vtkPVIsoVolume&);    // Not implemented.

  //ETX

protected:

  double VolumeFractionSurfaceValue;

  vtkPVIsoVolumeInternal* Implementation;
};

#endif // __vtkPVIsoVolume_h
