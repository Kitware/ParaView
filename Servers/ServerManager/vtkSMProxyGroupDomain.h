/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyGroupDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyGroupDomain -
// .SECTION Description
// .SECTION See Also

#ifndef __vtkSMProxyGroupDomain_h
#define __vtkSMProxyGroupDomain_h

#include "vtkSMDomain.h"

class vtkSMProperty;
class vtkSMProxy;

class VTK_EXPORT vtkSMProxyGroupDomain : public vtkSMDomain
{
public:
  static vtkSMProxyGroupDomain* New();
  vtkTypeRevisionMacro(vtkSMProxyGroupDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  vtkSetStringMacro(ProxyGroup);
  vtkGetStringMacro(ProxyGroup);

  // Description:
  virtual int IsInDomain(vtkSMProperty* property);
  int IsInDomain(vtkSMProxy* proxy);

protected:
  vtkSMProxyGroupDomain();
  ~vtkSMProxyGroupDomain();

  char* ProxyGroup;

  virtual void SaveState(const char*, ofstream*, vtkIndent) {};

private:
  vtkSMProxyGroupDomain(const vtkSMProxyGroupDomain&); // Not implemented
  void operator=(const vtkSMProxyGroupDomain&); // Not implemented
};

#endif
