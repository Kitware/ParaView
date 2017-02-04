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
/**
 * @class   vtkSMEnumerationDomain
 * @brief   list of integers with associated strings
 *
 * vtkSMEnumerationDomain represents an enumeration of integer values
 * with associated descriptive strings.
 * Valid XML elements are:
 * @verbatim
 * * <Entry text="" value=""/> where text is the descriptive
 * string and value is the integer value.
 * @endverbatim
*/

#ifndef vtkSMEnumerationDomain_h
#define vtkSMEnumerationDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"

struct vtkSMEnumerationDomainInternals;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMEnumerationDomain : public vtkSMDomain
{
public:
  static vtkSMEnumerationDomain* New();
  vtkTypeMacro(vtkSMEnumerationDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns true if the value of the propery is in the domain.
   * The propery has to be a vtkSMIntVectorProperty. If all
   * vector values are in the domain, it returns 1. It returns
   * 0 otherwise.
   */
  virtual int IsInDomain(vtkSMProperty* property) VTK_OVERRIDE;

  /**
   * Returns true if the int is in the domain. If value is
   * in domain, it's index is return in idx.
   */
  int IsInDomain(int val, unsigned int& idx);

  /**
   * Returns the number of entries in the enumeration.
   */
  unsigned int GetNumberOfEntries();

  /**
   * Returns the integer value of an enumeration entry.
   */
  int GetEntryValue(unsigned int idx);

  /**
   * Returns the descriptive string of an enumeration entry.
   */
  const char* GetEntryText(unsigned int idx);

  /**
   * Returns the text for an enumeration value.
   */
  const char* GetEntryTextForValue(int value);

  /**
   * Return 1 is the text is present in the enumeration, otherwise 0.
   */
  int HasEntryText(const char* text);

  /**
   * Get the value for an enumeration text. The return value is valid only is
   * HasEntryText() returns 1.
   */
  int GetEntryValueForText(const char* text);

  /**
   * Given an entry text, return the integer value.
   * Valid is set to 1 if text is defined, otherwise 0.
   * If valid=0, return value is undefined.
   */
  int GetEntryValue(const char* text, int& valid);

  /**
   * Add a new enumeration entry. text cannot be null.
   */
  void AddEntry(const char* text, int value);

  /**
   * Clear all entries.
   */
  void RemoveAllEntries();

  /**
   * Update self based on the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  virtual void Update(vtkSMProperty* property) VTK_OVERRIDE;

  //@{
  /**
   * Overridden to ensure that the property's default value is valid for the
   * enumeration, if not it will be set to the first enumeration value.
   */
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMEnumerationDomain();
  ~vtkSMEnumerationDomain();
  //@}

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  virtual void ChildSaveState(vtkPVXMLElement* domainElement) VTK_OVERRIDE;

  vtkSMEnumerationDomainInternals* EInternals;

private:
  vtkSMEnumerationDomain(const vtkSMEnumerationDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMEnumerationDomain&) VTK_DELETE_FUNCTION;
};

#endif
