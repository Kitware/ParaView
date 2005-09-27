/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfPartsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNumberOfPartsDomain - restricts the number of parts of input
// .SECTION Description
// vtkSMInputArrayDomain requires that the source proxy pointed by the
// property has an output with the specified multiplicity of parts:
// SINGLE or MULTIPLE.  Valid XML attributes are:
// @verbatim
// * multiplicity - can be either single or multiple
// @endverbatim
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMNumberOfPartsDomain_h
#define __vtkSMNumberOfPartsDomain_h

#include "vtkSMDomain.h"

class vtkSMSourceProxy;

class VTK_EXPORT vtkSMNumberOfPartsDomain : public vtkSMDomain
{
public:
  static vtkSMNumberOfPartsDomain* New();
  vtkTypeRevisionMacro(vtkSMNumberOfPartsDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. If the number of parts contained by
  // the source matches the criteria set in the domain, returns 1.
  // Returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // If the number of parts contained by
  // the source matches the criteria set in the domain, returns 1.
  // Returns 0 otherwise.
  int IsInDomain(vtkSMSourceProxy* proxy);

  // Description:
  // Set/get the part multiplicity. Can be either SINGLE or MULTIPLE.
  vtkSetMacro(PartMultiplicity, unsigned char);
  vtkGetMacro(PartMultiplicity, unsigned char);

//BTX
  enum NumberOfParts
  {
    SINGLE = 0,
    MULTIPLE = 1
  };
//ETX

protected:
  vtkSMNumberOfPartsDomain();
  ~vtkSMNumberOfPartsDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

  unsigned char PartMultiplicity;

private:
  vtkSMNumberOfPartsDomain(const vtkSMNumberOfPartsDomain&); // Not implemented
  void operator=(const vtkSMNumberOfPartsDomain&); // Not implemented
};

#endif
