/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDomain -
// .SECTION Description
// .SECTION See Also

#ifndef __vtkSMDomain_h
#define __vtkSMDomain_h

#include "vtkSMObject.h"

class vtkSMProperty;

class VTK_EXPORT vtkSMDomain : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMDomain, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual int IsInDomain(vtkSMProperty* property) = 0;

protected:
  vtkSMDomain();
  ~vtkSMDomain();

private:
  vtkSMDomain(const vtkSMDomain&); // Not implemented
  void operator=(const vtkSMDomain&); // Not implemented
};

#endif
