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
// .NAME vtkSMDoubleRangeDomain - double interval specified by min and max
// .SECTION Description
// vtkSMDoubleRangeDomain represents an interval in real space (using
// double precision) specified using a min and a max value.
// Valid XML attributes are:
// @verbatim
// * min 
// * max
// @endverbatim
// Both min and max attributes can have one or more space space
// separated (double) arguments.
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
  // 0 otherwise. A value is in the domain if it is between (min, max)
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the int is in the domain. If value is
  // in domain, it's index is return in idx.
  // A value is in the domain if it is between (min, max)
  int IsInDomain(unsigned int idx, double val);

  // Description:
  // Return a min. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified min. is equivalent to -inf
  double GetMinimum(unsigned int idx, int& exists);

  // Description:
  // Return a max. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified max. is equivalent to inf
  double GetMaximum(unsigned int idx, int& exists);

  // Description:
  // Set a min. of a given index.
  void AddMinimum(unsigned int idx, double value);

  // Description:
  // Remove a min. of a given index.
  // An unspecified min. is equivalent to -inf
  void RemoveMinimum(unsigned int idx);

  // Description:
  // Clear all minimum values.
  void RemoveAllMinima();

  // Description:
  // Set a max. of a given index.
  void AddMaximum(unsigned int idx, double value);

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
  
  // Description:
  // Set the value of an element of a property from the animation editor in
  // a batch script.
  virtual void SetAnimationValueInBatch(ofstream *file,
                                        vtkSMProperty *property,
                                        vtkClientServerID sourceID,
                                        int idx, double value);
  
protected:
  vtkSMDoubleRangeDomain();
  ~vtkSMDoubleRangeDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

  // Description:
  // General purpose method called by both AddMinimum() and AddMaximum()
  void SetEntry(unsigned int idx, int minOrMax, int set, double value);

  // Internal use only.
  // Set/Get the number of min/max entries.
  unsigned int GetNumberOfEntries();
  void SetNumberOfEntries(unsigned int size);

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
