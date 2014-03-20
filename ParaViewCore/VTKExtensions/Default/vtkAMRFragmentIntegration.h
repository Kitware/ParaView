/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRFragmentIntegration.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
// .NAME vtkAMRFragmentIntegration - Generates fragment analysis from an
// amr volume and a previously run contour on that volume
//
// .SECTION Description
//   Input 0: The AMR Volume
//
//   Output 0: A multiblock containing tables of fragments, one block 
//             for each requested material

#ifndef __vtkAMRFragmentIntegration_h
#define __vtkAMRFragmentIntegration_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkMultiBlockDataSetAlgorithm.h"
#include <string>  // STL required.
#include <vector>  // STL required.

class vtkTable;
class vtkNonOverlappingAMR;
class vtkDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRFragmentIntegration : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRFragmentIntegration *New();
  vtkTypeMacro(vtkAMRFragmentIntegration,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkAMRFragmentIntegration();
  virtual ~vtkAMRFragmentIntegration();

  //BTX
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  //   Pipeline helper.  Run on each material independently.
  vtkTable* DoRequestData(vtkNonOverlappingAMR* volume, 
                          const char* volumeArray,
                          const char* massArray,
                          std::vector<std::string> volumeWeightedNames,
                          std::vector<std::string> massWeightedNames);

private:
  vtkAMRFragmentIntegration(const vtkAMRFragmentIntegration&);  // Not implemented.
  void operator=(const vtkAMRFragmentIntegration&);  // Not implemented.
  //ETX
};

#endif /* __vtkAMRFragmentIntegration_h */
//
// VTK-HeaderTest-Exclude: vtkAMRFragmentIntegration.h
