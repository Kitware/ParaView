/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangeDomainTemplate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRangeDomainTemplate - superclass for type-specific range domains
// i.e. domains that constrain a value within a min and max.
// .SECTION Description
// vtkSMRangeDomainTemplate represents an interval in real space (using
// precision based on the data-type) specified using a min and a max value.
// Valid XML attributes are:
// @verbatim
// * min 
// * max
// @endverbatim
// Both min and max attributes can have one or more space space
// separated value arguments.
// Optionally, a Required Property may be specified (which typically is a
// information property) which can be used to obtain the range for the values as
// follows:
// @verbatim
// <DoubleRangeDomain ...>
//    <RequiredProperties>
//      <Property name="<InfoPropName>" function="RangeInfo" />
//    </RequiredProperties>
// </DoubleRangeDomain>
// @endverbatim
//
// vtkSMRangeDomainTemplate provides a mechanism to control how the default
// value for any property can be determined using the domain either the min, max
// or mid of the range. One can do that using the "default_mode" attribute in
// XML with valid values as "min", "max" or "mid". If none is specified, "mid"
// is assumed.

#ifndef __vtkSMRangeDomainTemplate_h
#define __vtkSMRangeDomainTemplate_h

#include "vtkSMDomain.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkTuple.h" // needed for vtkTuple.
#include <vector> // needed for std::vector

template <class T>
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMRangeDomainTemplate :
  public vtkSMDomain
{
public:
  typedef vtkSMDomain Superclass;
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the properyy is in the domain.
  // If all vector values are in the domain, it returns 1. It returns
  // 0 otherwise. A value is in the domain if it is between (min, max).
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the double (val) is in the domain. If value is
  // in domain, it's index is return in idx.
  // A value is in the domain if it is between (min, max)
  bool IsInDomain(unsigned int idx, T val);

  // Description:
  // Return a min. value if it exists. If the min. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified min. is equivalent to -inf
  T GetMinimum(unsigned int idx, int& exists);

  // Description:
  // Return a max. value if it exists. If the max. exists
  // exists is set to 1. Otherwise, it is set to 0.
  // An unspecified max. is equivalent to +inf
  T GetMaximum(unsigned int idx, int& exists);

  // Description:
  // Returns if minimum/maximum bound is set for the domain.
  bool GetMinimumExists(unsigned int idx);
  bool GetMaximumExists(unsigned int idx);

  // Description:
  // Returns the minimum/maximum value, is exists, otherwise
  // 0 is returned. Use GetMaximumExists() GetMaximumExists() to make sure that
  // the bound is set.
  T GetMinimum(unsigned int idx)
    { int not_used; return this->GetMinimum(idx, not_used); }
  T GetMaximum(unsigned int idx)
    { int not_used; return this->GetMaximum(idx, not_used); }

  // Description:
  // Returns the number of entries in the internal
  // maxima/minima list. No maxima/minima exists beyond
  // this index. Maxima/minima below this number may or
  // may not exist.
  unsigned int GetNumberOfEntries();

  // Description:
  // Update self checking the "unchecked" values of all required
  // properties.
  virtual void Update(vtkSMProperty*);
  
  // Description:
  // Set the value of an element of a property from the animation editor.
  virtual void SetAnimationValue(
    vtkSMProperty *property, int idx, double value);

  enum DefaultModes
    {
    MIN,
    MAX,
    MID
    };

  // Description:
  // Get the default-mode that controls how SetDefaultValues() behaves.
  int GetDefaultMode();

  // Description:
  // Set the property's default value based on the domain. How the value is
  // determined using the range is controlled by DefaultMode.
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values);

//BTX
protected:
  vtkSMRangeDomainTemplate();
  ~vtkSMRangeDomainTemplate();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  struct vtkEntry
    {
    vtkTuple<T, 2> Value;
    vtkTuple<bool, 2> Valid;
    vtkEntry()
      {
      this->Value[0] = this->Value[1] = 0;
      this->Valid[0] = this->Valid[1] = false;
      }
    vtkEntry(T min, bool minValid, T max, bool maxValid)
      {
      this->Value[0] = min; this->Value[1] = max;
      this->Valid[0] = minValid; this->Valid[1] = maxValid;
      }
    vtkEntry(T min, T max)
      {
      this->Value[0] = min; this->Value[1] = max;
      this->Valid[0] = this->Valid[1] = true;
      }

    bool operator==(const vtkEntry& other) const
      {
      return this->Valid == other.Valid && this->Value == other.Value;
      }
    };

  // We keep Entries private so we can carefully manage firing the modified
  // events since subclasses can often forget the minutia.
  const std::vector<vtkEntry>& GetEntries() const
    { return this->Entries; }
  void SetEntries(const std::vector<vtkEntry>& new_value)
    {
    typedef typename std::vector<vtkEntry>::const_iterator cit;
    cit b = this->Entries.begin();
    cit e = this->Entries.end();
    if (this->Entries.size() != new_value.size() ||
      !std::equal(b,e, new_value.begin()))
      {
      this->Entries = new_value;
      this->DomainModified();
      }
    }

  int DefaultMode;

private:
  vtkSMRangeDomainTemplate(const vtkSMRangeDomainTemplate&); // Not implemented
  void operator=(const vtkSMRangeDomainTemplate&); // Not implemented

  bool GetComputedDefaultValue(unsigned int index, T& value);

  std::vector<vtkEntry> Entries;
//ETX
};

#if !defined(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
# define VTK_SM_RANGE_DOMAIN_TEMPLATE_INSTANTIATE(T) \
   template class VTKPVSERVERMANAGERCORE_EXPORT vtkSMRangeDomainTemplate< T >
#else
# include "vtkSMRangeDomainTemplate.txx" // needed for templates.
# define VTK_SM_RANGE_DOMAIN_TEMPLATE_INSTANTIATE(T)
#endif // !defined(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)

#endif // !defined(__vtkSMRangeDomainTemplate_h)

// This portion must be OUTSIDE the include blockers.  Each
// vtkSMRangeDomainTemplate subclass uses this to give its instantiation
// of this template a DLL interface.
#if defined(VTK_SM_RANGE_DOMAIN_TEMPLATE_TYPE)
# if defined(VTK_BUILD_SHARED_LIBS) && defined(_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable: 4091) // warning C4091: 'extern ' :
   // ignored on left of 'int' when no variable is declared
#  pragma warning (disable: 4231) // Compiler-specific extension warning.
   // Use an "extern explicit instantiation" to give the class a DLL
   // interface.  This is a compiler-specific extension.
   extern VTK_SM_RANGE_DOMAIN_TEMPLATE_INSTANTIATE(VTK_SM_RANGE_DOMAIN_TEMPLATE_TYPE);
#  pragma warning (pop)
# endif
# undef VTK_SM_RANGE_DOMAIN_TEMPLATE_TYPE
#endif

// VTK-HeaderTest-Exclude: vtkSMRangeDomainTemplate.h
