/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCustomBoundsDomain
// .SECTION Description
//

#ifndef __vtkSMCustomBoundsDomain_h
#define __vtkSMCustomBoundsDomain_h

#include "vtkSMBoundsDomain.h"

class VTK_EXPORT vtkSMCustomBoundsDomain : public vtkSMBoundsDomain
{
public:
  static vtkSMCustomBoundsDomain* New();
  vtkTypeMacro(vtkSMCustomBoundsDomain, vtkSMBoundsDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  virtual int SetDefaultValues(vtkSMProperty*);

//BTX
protected:
  vtkSMCustomBoundsDomain();
  ~vtkSMCustomBoundsDomain();

  virtual void UpdateFromInformation(vtkPVDataInformation* information);

private:
  vtkSMCustomBoundsDomain(const vtkSMCustomBoundsDomain&); // Not implemented
  void operator=(const vtkSMCustomBoundsDomain&); // Not implemented
//ETX
};

#endif
