/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputArrayDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInputArrayDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMInputArrayDomain_h
#define __vtkSMInputArrayDomain_h

#include "vtkSMDomain.h"

class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMInputArrayDomain : public vtkSMDomain
{
public:
  static vtkSMInputArrayDomain* New();
  vtkTypeRevisionMacro(vtkSMInputArrayDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. The input has to have one or more
  // arrays that match the requirements.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if input has one or more arrays that match the
  // requirements
  int IsInDomain(vtkSMSourceProxy* proxy);

  // Description:
  int IsFieldValid(vtkPVArrayInformation* arrayInfo);
  
  // Description:
  vtkSetMacro(AttributeType, unsigned char);
  vtkGetMacro(AttributeType, unsigned char);
  const char* GetAttributeTypeAsString();
  virtual void SetAttributeType(const char* type);

  // Description:
  vtkSetMacro(NumberOfComponents, int);
  vtkGetMacro(NumberOfComponents, int);

//BTX
  enum AttributeTypes
  {
    POINT = 0,
    CELL = 1,
    ANY = 2,
    LAST_ATTRIBUTE_TYPE
  };
//ETX

protected:
  vtkSMInputArrayDomain();
  ~vtkSMInputArrayDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  int AttributeInfoContainsArray(vtkPVDataSetAttributesInformation* attrInfo);

  unsigned char AttributeType;
  int NumberOfComponents;

private:
  vtkSMInputArrayDomain(const vtkSMInputArrayDomain&); // Not implemented
  void operator=(const vtkSMInputArrayDomain&); // Not implemented
};

#endif
