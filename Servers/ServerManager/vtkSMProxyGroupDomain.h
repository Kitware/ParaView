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
//BTX
struct vtkSMProxyGroupDomainInternals;
//ETX

class VTK_EXPORT vtkSMProxyGroupDomain : public vtkSMDomain
{
public:
  static vtkSMProxyGroupDomain* New();
  vtkTypeRevisionMacro(vtkSMProxyGroupDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void AddGroup(const char* group);

  // Description:
  virtual int IsInDomain(vtkSMProperty* property);
  int IsInDomain(vtkSMProxy* proxy);

  // Description:
  unsigned int GetNumberOfGroups();

  // Description:
  const char* GetGroup(unsigned int idx);

protected:
  vtkSMProxyGroupDomain();
  ~vtkSMProxyGroupDomain();

  virtual void SaveState(const char*, ofstream*, vtkIndent) {};

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  vtkSMProxyGroupDomainInternals* PGInternals;

private:
  vtkSMProxyGroupDomain(const vtkSMProxyGroupDomain&); // Not implemented
  void operator=(const vtkSMProxyGroupDomain&); // Not implemented
};

#endif
