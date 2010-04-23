/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHardwareSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVHardwareSelector - vtkHardwareSelector subclass with logic to work
// in Parallel.
// .SECTION Description
//

#ifndef __vtkPVHardwareSelector_h
#define __vtkPVHardwareSelector_h

#include "vtkHardwareSelector.h"

class VTK_EXPORT vtkPVHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkPVHardwareSelector* New();
  vtkTypeMacro(vtkPVHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  void BeginSelection()
    { this->Superclass::BeginSelection(); }
  void SetCurrentPass(int pass)
    { this->CurrentPass = pass; }
  void EndSelection()
    { this->Superclass::EndSelection(); }

  vtkSetMacro(NumberOfIDs, vtkIdType);
  vtkGetMacro(NumberOfIDs, vtkIdType);

  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);

//BTX
protected:
  vtkPVHardwareSelector();
  ~vtkPVHardwareSelector();

  // Description:
  // Return a unique ID for the prop.
  virtual int GetPropID(int idx, vtkProp* prop);

  // Description:
  // Returns is the pass indicated is needed.
  virtual bool PassRequired(int pass);

  int NumberOfProcesses;
  vtkIdType NumberOfIDs;
private:
  vtkPVHardwareSelector(const vtkPVHardwareSelector&); // Not implemented
  void operator=(const vtkPVHardwareSelector&); // Not implemented
//ETX
};

#endif

