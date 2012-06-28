/*=========================================================================

  Program:   ParaView
  Module:    vtkCompleteArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompleteArrays - Filter adds arrays to empty partitions.
// .SECTION Description
// This is a temporary solution for fixing a writer bug.  When partition 0
// has no cells or points, it does not have arrays either.  The writers
// get confused.  This filter creates empty arrays on node zero if there
// are no cells or points in that partition.

#ifndef __vtkCompleteArrays_h
#define __vtkCompleteArrays_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;
class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkDataSetAttributes;


class VTK_EXPORT vtkCompleteArrays : public vtkDataSetAlgorithm 
{
public:
  vtkTypeMacro(vtkCompleteArrays,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
  // range is (0,1).
  static vtkCompleteArrays *New();

  // Description:
  // The user can set the controller used for inter-process communication.
  void SetController(vtkMultiProcessController *controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkCompleteArrays();
  ~vtkCompleteArrays();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  void FillArrays(vtkDataSetAttributes* da, 
                  vtkPVDataSetAttributesInformation* attrInfo);

  vtkMultiProcessController* Controller;

  vtkDataArray* CreateArray(vtkPVArrayInformation* aInfo);

private:
  vtkCompleteArrays(const vtkCompleteArrays&);  // Not implemented.
  void operator=(const vtkCompleteArrays&);  // Not implemented.
};

#endif


