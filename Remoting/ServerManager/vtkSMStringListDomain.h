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
/**
 * @class   vtkSMStringListDomain
 * @brief   list of strings
 *
 * vtkSMStringListDomain represents a domain consisting of a list of
 * strings. It only works with vtkSMStringVectorProperty.
 *
 * Supported attributes:
 * \li \c none_string: (optional) when specified, this string appears as the
 *                first entry in the domain the list and can be used to show
 *                "None", or "Not available" etc.
 *
 * Valid XML elements are:
 * @verbatim
 * * <String value="">
 * @endverbatim
 * @sa
 * vtkSMDomain vtkSMStringVectorProperty
*/

#ifndef vtkSMStringListDomain_h
#define vtkSMStringListDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"
#include <vector> //  needed for vector.

struct vtkSMStringListDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMStringListDomain : public vtkSMDomain
{
public:
  static vtkSMStringListDomain* New();
  vtkTypeMacro(vtkSMStringListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMStringVectorProperty. If all
   * vector values are in the domain, it returns 1. It returns
   * 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns true if the string is in the domain.
   */
  int IsInDomain(const char* string, unsigned int& idx);

  /**
   * Returns a string in the domain. The pointer may become
   * invalid once the domain has been modified.
   */
  const char* GetString(unsigned int idx);

  /**
   * Returns the number of strings in the domain.
   */
  unsigned int GetNumberOfStrings();

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

  /**
   * Set the value of an element of a property from the animation editor.
   */
  void SetAnimationValue(vtkSMProperty*, int, double) override;

  //@{
  /**
   * Return the string that is used as "none_string" in XML configuration.
   */
  vtkGetStringMacro(NoneString);
  vtkSetStringMacro(NoneString);
  //@}

  //@{
  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * Returns 1 if the domain updated the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMStringListDomain();
  ~vtkSMStringListDomain() override;
  //@}

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  void ChildSaveState(vtkPVXMLElement* domainElement) override;

  /**
   * Default string always present in this string list. When selected, it is equivalent
   * with not selecting any string.
   */
  char* NoneString;

  //@{
  /**
   * Call to set the strings. Will fire DomainModifiedEvent if the domain values
   * have indeed changed.
   */
  virtual void SetStrings(const std::vector<std::string>& strings);
  const std::vector<std::string>& GetStrings();
  //@}

private:
  vtkSMStringListDomain(const vtkSMStringListDomain&) = delete;
  void operator=(const vtkSMStringListDomain&) = delete;

  vtkSMStringListDomainInternals* SLInternals;
};

#endif
