/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAMRFragmentIntegration.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
// .NAME vtkPVAMRFragmentIntegration - Generates fragment analysis from an
// amr volume and a previously run contour on that volume
//
// .SECTION Description
//
// .SEE vtkAMRFragmentIntegration
//

#ifndef __vtkPVAMRFragmentIntegration_h
#define __vtkPVAMRFragmentIntegration_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkAMRFragmentIntegration.h"

// Forware declaration.
class vtkPVAMRFragmentIntegrationInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVAMRFragmentIntegration : public vtkAMRFragmentIntegration
{
public:
  static vtkPVAMRFragmentIntegration* New();
  vtkTypeMacro(vtkPVAMRFragmentIntegration,vtkAMRFragmentIntegration);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkPVAMRFragmentIntegration();
  virtual ~vtkPVAMRFragmentIntegration();

  // Description:
  // Add to list of volume arrays which are used for generating contours.
  void AddInputVolumeArrayToProcess(const char* name);
  void ClearInputVolumeArrayToProcess();

  // Description:
  // Add to list of mass arrays 
  void AddInputMassArrayToProcess(const char* name);
  void ClearInputMassArrayToProcess();

  // Description:
  // Add to list of volume weighted arrays 
  void AddInputVolumeWeightedArrayToProcess(const char* name);
  void ClearInputVolumeWeightedArrayToProcess();

  // Description:
  // Add to list of mass weighted arrays 
  void AddInputMassWeightedArrayToProcess(const char* name);
  void ClearInputMassWeightedArrayToProcess();

  void SetContourConnection (vtkAlgorithmOutput*);

  //BTX
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

private:
  vtkPVAMRFragmentIntegration(const vtkPVAMRFragmentIntegration&);  // Not implemented.
  void operator=(const vtkPVAMRFragmentIntegration&);    // Not implemented.

  //ETX

protected:
  double VolumeFractionSurfaceValue;
  vtkPVAMRFragmentIntegrationInternal* Implementation;
};

#endif // __vtkPVAMRFragmentIntegration_h
