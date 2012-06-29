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

#include "vtkSMIntRangeDomain.h"

class vtkSMProxyProperty;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMNumberOfGroupsDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMNumberOfGroupsDomain* New();
  vtkTypeMacro(vtkSMNumberOfGroupsDomain, vtkSMIntRangeDomain);
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
  int IsInDomain(vtkSMSourceProxy* proxy, int outputport=0);

  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

  // Description:
  // Set/get the group multiplicity. Can be either SINGLE or MULTIPLE.
  vtkSetMacro(GroupMultiplicity, unsigned char);
  vtkGetMacro(GroupMultiplicity, unsigned char);

//BTX
  enum NumberOfGroups
  {
    NOT_SET = 0,
    SINGLE = 1,
    MULTIPLE = 2
  };
//ETX

protected:
  vtkSMNumberOfGroupsDomain();
  ~vtkSMNumberOfGroupsDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);

  void Update(vtkSMProxyProperty *pp);

  unsigned char GroupMultiplicity;

private:
  vtkSMNumberOfGroupsDomain(const vtkSMNumberOfGroupsDomain&); // Not implemented
  void operator=(const vtkSMNumberOfGroupsDomain&); // Not implemented
};

#endif
