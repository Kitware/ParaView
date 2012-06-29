/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringListRangeDomain - domain for string lists that also have ranges
// .SECTION Description
// vtkSMStringListRangeDomain restricts the values of string vectors
// (works only with vtkSMStringVectorProperty) to a list of strings
// and either a range or a boolean. This is used with string properties
// that have tuples of string and int type components. A good example
// is array selection, where the first entry is the name of the array
// (string) and the second one is whether it is selected or not (int, bool).
// Another example is xdmf parameters, where the first entry is the
// name of the property and the second one it's value (restricted to
// an int range)
// .Section See Also
// vtkSMIntRangeDomain vtkSMBooleanDomain vtkSMStringListDomain

#ifndef __vtkSMStringListRangeDomain_h
#define __vtkSMStringListRangeDomain_h

#include "vtkSMDomain.h"

class vtkSMIntRangeDomain;
class vtkSMBooleanDomain;
class vtkSMStringListDomain;

class VTK_EXPORT vtkSMStringListRangeDomain : public vtkSMDomain
{
public:
  static vtkSMStringListRangeDomain* New();
  vtkTypeMacro(vtkSMStringListRangeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // True if every even element is in the list of strings and
  // every add element is in the range (or boolean), false
  // otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  //BTX
  enum Modes
  {
    RANGE,
    BOOLEAN
  };
  //ETX

  // Description:
  // Set the domain for the integer value. Can be either RANGE
  // or BOOLEAN
  vtkSetClampMacro(IntDomainMode, int, 0, 1);
  vtkGetMacro(IntDomainMode, int);

  // Description:
  // Returns the number of strings in the domain.
  unsigned int GetNumberOfStrings();

  // Description:
  // Returns a string in the domain. The pointer may become
  // invalid once the domain has been modified.
  const char* GetString(unsigned int idx);

  // Description:
  // Adds a new string to the domain.
  unsigned int AddString(const char* string);

  // Description:
  // Removes a string from the domain.
  void RemoveString(const char* string);

  // Description:
  // Removes all strings from the domain.
  void RemoveAllStrings();

  // Description:
  // Return a min. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified min. is equivalent to -inf
  int GetMinimum(unsigned int idx, int& exists);

  // Description:
  // Return a max. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified max. is equivalent to inf
  int GetMaximum(unsigned int idx, int& exists);

  // Description:
  // Set a min. of a given index.
  void AddMinimum(unsigned int idx, int value);

  // Description:
  // Remove a min. of a given index.
  // An unspecified min. is equivalent to -inf
  void RemoveMinimum(unsigned int idx);

  // Description:
  // Clear all minimum values.
  void RemoveAllMinima();

  // Description:
  // Set a max. of a given index.
  void AddMaximum(unsigned int idx, int value);

  // Description:
  // Remove a max. of a given index.
  // An unspecified min. is equivalent to inf
  void RemoveMaximum(unsigned int idx);

  // Description:
  // Clear all maximum values.
  void RemoveAllMaxima();

  // Description:
  // Set the value of an element of a property from the animation editor.
  virtual void SetAnimationValue(vtkSMProperty *property, int idx,
                                 double value);

protected:
  vtkSMStringListRangeDomain();
  ~vtkSMStringListRangeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);

  vtkSMIntRangeDomain* IRDomain;
  vtkSMBooleanDomain* BDomain;
  vtkSMStringListDomain* SLDomain;

  int IntDomainMode;

private:
  vtkSMStringListRangeDomain(const vtkSMStringListRangeDomain&); // Not implemented
  void operator=(const vtkSMStringListRangeDomain&); // Not implemented
};

#endif
