/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataTypeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataTypeDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMDataTypeDomain_h
#define __vtkSMDataTypeDomain_h

#include "vtkSMDomain.h"

class vtkSMSourceProxy;
//BTX
struct vtkSMDataTypeDomainInternals;
//ETX

class VTK_EXPORT vtkSMDataTypeDomain : public vtkSMDomain
{
public:
  static vtkSMDataTypeDomain* New();
  vtkTypeRevisionMacro(vtkSMDataTypeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. If all data types of the input's
  // parts are in the domain, it returns. It returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if all parts of the source proxy are in the domain.
  int IsInDomain(vtkSMSourceProxy* proxy);

  // Description:
  unsigned int GetNumberOfDataTypes();

  // Description:
  const char* GetDataType(unsigned int idx);

protected:
  vtkSMDataTypeDomain();
  ~vtkSMDataTypeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  vtkSMDataTypeDomainInternals* DTInternals;

private:
  vtkSMDataTypeDomain(const vtkSMDataTypeDomain&); // Not implemented
  void operator=(const vtkSMDataTypeDomain&); // Not implemented
};

#endif
