/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfGroupsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNumberOfGroupsDomain - restricts the number of groups of input
// .SECTION Description
// vtkSMNumberOfGroupsDomain requires that the source proxy pointed by the
// property has an output with the specified multiplicity of groups:
// SINGLE or MULTIPLE.  Valid XML attributes are:
// @verbatim
// * multiplicity - can be either single or multiple
// @endverbatim
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMNumberOfGroupsDomain_h
#define __vtkSMNumberOfGroupsDomain_h

#include "vtkSMDomain.h"

class vtkSMSourceProxy;

class VTK_EXPORT vtkSMNumberOfGroupsDomain : public vtkSMDomain
{
public:
  static vtkSMNumberOfGroupsDomain* New();
  vtkTypeRevisionMacro(vtkSMNumberOfGroupsDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. If the number of groups contained by
  // the source matches the criteria set in the domain, returns 1.
  // Returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // If the number of groups contained by
  // the source matches the criteria set in the domain, returns 1.
  // Returns 0 otherwise.
  int IsInDomain(vtkSMSourceProxy* proxy);

  // Description:
  // Set/get the group multiplicity. Can be either SINGLE or MULTIPLE.
  vtkSetMacro(GroupMultiplicity, unsigned char);
  vtkGetMacro(GroupMultiplicity, unsigned char);

//BTX
  enum NumberOfGroups
  {
    SINGLE = 0,
    MULTIPLE = 1
  };
//ETX

protected:
  vtkSMNumberOfGroupsDomain();
  ~vtkSMNumberOfGroupsDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

  unsigned char GroupMultiplicity;

private:
  vtkSMNumberOfGroupsDomain(const vtkSMNumberOfGroupsDomain&); // Not implemented
  void operator=(const vtkSMNumberOfGroupsDomain&); // Not implemented
};

#endif
