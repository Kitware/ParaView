/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDoubleRangeDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMDoubleRangeDomain_h
#define __vtkSMDoubleRangeDomain_h

#include "vtkSMDomain.h"

//BTX
struct vtkSMDoubleRangeDomainInternals;
//ETX

class VTK_EXPORT vtkSMDoubleRangeDomain : public vtkSMDomain
{
public:
  static vtkSMDoubleRangeDomain* New();
  vtkTypeRevisionMacro(vtkSMDoubleRangeDomain, vtkSMDomain);
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
  int IsInDomain(unsigned int idx, double val);

  // Description:
  double GetMinimum(unsigned int idx, int& exists);

  // Description:
  double GetMaximum(unsigned int idx, int& exists);

  // Description:
  void AddMinimum(unsigned int idx, double value);

  // Description:
  void RemoveMinimum(unsigned int idx);

  // Description:
  void AddMaximum(unsigned int idx, double value);

  // Description:
  void RemoveMaximum(unsigned int idx);

protected:
  vtkSMDoubleRangeDomain();
  ~vtkSMDoubleRangeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  void SetEntry(unsigned int idx, int minOrMax, int set, double value);

  vtkSMDoubleRangeDomainInternals* DRInternals;

//BTX
  enum
  {
    MIN = 0,
    MAX = 1
  };
//ETX

private:
  vtkSMDoubleRangeDomain(const vtkSMDoubleRangeDomain&); // Not implemented
  void operator=(const vtkSMDoubleRangeDomain&); // Not implemented
};

#endif
