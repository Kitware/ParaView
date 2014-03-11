/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringListDomain - list of strings
// .SECTION Description
// vtkSMStringListDomain represents a domain consisting of a list of
// strings. It only works with vtkSMStringVectorProperty. 
// Valid XML elements are:
// @verbatim
// * <String value="">
// @endverbatim
// .SECTION See Also
// vtkSMDomain vtkSMStringVectorProperty

#ifndef __vtkSMStringListDomain_h
#define __vtkSMStringListDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include <vector> //  needed for vector.
class vtkStdString;
//BTX
struct vtkSMStringListDomainInternals;
//ETX

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMStringListDomain : public vtkSMDomain
{
public:
  static vtkSMStringListDomain* New();
  vtkTypeMacro(vtkSMStringListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the property is in the domain.
  // The propery has to be a vtkSMStringVectorProperty. If all 
  // vector values are in the domain, it returns 1. It returns
  // 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the string is in the domain.
  int IsInDomain(const char* string, unsigned int& idx);

  // Description:
  // Returns a string in the domain. The pointer may become
  // invalid once the domain has been modified.
  const char* GetString(unsigned int idx);

  // Description:
  // Returns the number of strings in the domain.
  unsigned int GetNumberOfStrings();

  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

  // Description:
  // Set the value of an element of a property from the animation editor.
  virtual void SetAnimationValue(vtkSMProperty*, int, double);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  // Returns 1 if the domain updated the property.
  virtual int SetDefaultValues(vtkSMProperty*);
protected:
  vtkSMStringListDomain();
  ~vtkSMStringListDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);
  
  // Load the state of the domain from the XML.
  virtual int LoadState(vtkPVXMLElement* domainElement, 
    vtkSMProxyLocator* loader);

  // Description:
  // Call to set the strings. Will fire DomainModifiedEvent if the domain values
  // have indeed changed.
  void SetStrings(const std::vector<vtkStdString>& strings);
  const std::vector<vtkStdString>& GetStrings();

private:
  vtkSMStringListDomain(const vtkSMStringListDomain&); // Not implemented
  void operator=(const vtkSMStringListDomain&); // Not implemented
  
  vtkSMStringListDomainInternals* SLInternals;
};

#endif
