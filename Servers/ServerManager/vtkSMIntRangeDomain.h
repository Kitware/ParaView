/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIntRangeDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMIntRangeDomain_h
#define __vtkSMIntRangeDomain_h

#include "vtkSMDomain.h"

//BTX
struct vtkSMIntRangeDomainInternals;
//ETX

class VTK_EXPORT vtkSMIntRangeDomain : public vtkSMDomain
{
public:
  static vtkSMIntRangeDomain* New();
  vtkTypeRevisionMacro(vtkSMIntRangeDomain, vtkSMDomain);
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
  int IsInDomain(unsigned int idx, int val);

  // Description:
  int GetMinimum(unsigned int idx, int& exists);

  // Description:
  int GetMaximum(unsigned int idx, int& exists);

  // Description:
  void AddMinimum(unsigned int idx, int value);

  // Description:
  void RemoveMinimum(unsigned int idx);

  // Description:
  void AddMaximum(unsigned int idx, int value);

  // Description:
  void RemoveMaximum(unsigned int idx);

protected:
  vtkSMIntRangeDomain();
  ~vtkSMIntRangeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  void SetEntry(unsigned int idx, int minOrMax, int set, int value);

  vtkSMIntRangeDomainInternals* IRInternals;

//BTX
  enum
  {
    MIN = 0,
    MAX = 1
  };
//ETX

private:
  vtkSMIntRangeDomain(const vtkSMIntRangeDomain&); // Not implemented
  void operator=(const vtkSMIntRangeDomain&); // Not implemented
};

#endif
