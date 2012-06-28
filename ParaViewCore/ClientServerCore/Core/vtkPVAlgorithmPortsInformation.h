/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAlgorithmPortsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAlgorithmPortsInformation - Holds number of outputs
// .SECTION Description
// This information object collects the number of outputs from the
// sources.  This is separate from vtkPVDataInformation because the number of
// outputs can be determined before Update is called.

#ifndef __vtkPVAlgorithmPortsInformation_h
#define __vtkPVAlgorithmPortsInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVAlgorithmPortsInformation : public vtkPVInformation
{
public:
  static vtkPVAlgorithmPortsInformation* New();
  vtkTypeMacro(vtkPVAlgorithmPortsInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get number of outputs for a particular source.
  vtkGetMacro(NumberOfOutputs, int);

  // Description:
  // Get the number of required inputs for a particular algorithm.
  vtkGetMacro(NumberOfRequiredInputs, int);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVAlgorithmPortsInformation();
  ~vtkPVAlgorithmPortsInformation();

  int NumberOfOutputs;
  int NumberOfRequiredInputs;

  vtkSetMacro(NumberOfOutputs, int);
private:
  vtkPVAlgorithmPortsInformation(const vtkPVAlgorithmPortsInformation&); // Not implemented
  void operator=(const vtkPVAlgorithmPortsInformation&); // Not implemented
};

#endif
