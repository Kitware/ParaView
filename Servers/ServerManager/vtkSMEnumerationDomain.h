/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnumerationDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMEnumerationDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMEnumerationDomain_h
#define __vtkSMEnumerationDomain_h

#include "vtkSMDomain.h"

//BTX
struct vtkSMEnumerationDomainInternals;
//ETX

class VTK_EXPORT vtkSMEnumerationDomain : public vtkSMDomain
{
public:
  static vtkSMEnumerationDomain* New();
  vtkTypeRevisionMacro(vtkSMEnumerationDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMIntVectorProperty. If all 
  // vector values are in the domain, it returns 1. It returns
  // 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the int is in the domain. If value is
  // in domain, it's index is return in idx.
  int IsInDomain(int val, unsigned int& idx);

  // Description:
  unsigned int GetNumberOfEntries();

  // Description:
  int GetEntryValue(unsigned int idx);

  // Description:
  const char* GetEntryText(unsigned int idx);

  // Description:
  void AddEntry(const char* text, int value);

  // Description:
  void RemoveAllEntries();

protected:
  vtkSMEnumerationDomain();
  ~vtkSMEnumerationDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  vtkSMEnumerationDomainInternals* EInternals;

private:
  vtkSMEnumerationDomain(const vtkSMEnumerationDomain&); // Not implemented
  void operator=(const vtkSMEnumerationDomain&); // Not implemented
};

#endif
