/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNumberOfOutputsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVNumberOfOutputsInformation - Holds number of outputs
// .SECTION Description
// This information object collects the number of outputs from the
// sources.  This is separate from vtkPVDataInformation because the number of
// outputs can be determined before Update is called.

#ifndef __vtkPVNumberOfOutputsInformation_h
#define __vtkPVNumberOfOutputsInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVNumberOfOutputsInformation : public vtkPVInformation
{
public:
  static vtkPVNumberOfOutputsInformation* New();
  vtkTypeRevisionMacro(vtkPVNumberOfOutputsInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get number of outputs for a particular source.
  vtkGetMacro(NumberOfOutputs, int);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*) const;
  virtual void CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVNumberOfOutputsInformation();
  ~vtkPVNumberOfOutputsInformation();

  int NumberOfOutputs;
  vtkSetMacro(NumberOfOutputs, int);
private:
  vtkPVNumberOfOutputsInformation(const vtkPVNumberOfOutputsInformation&); // Not implemented
  void operator=(const vtkPVNumberOfOutputsInformation&); // Not implemented
};

#endif
