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
 * Valid XML elements are:
 * @verbatim
 * * <String value="">
 * @endverbatim
 * @sa
 * vtkSMDomain vtkSMStringVectorProperty
*/

#ifndef vtkSMStringListDomain_h
#define vtkSMStringListDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"
#include <vector> //  needed for vector.
class vtkStdString;

struct vtkSMStringListDomainInternals;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMStringListDomain : public vtkSMDomain
{
public:
  static vtkSMStringListDomain* New();
  vtkTypeMacro(vtkSMStringListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns true if the value of the property is in the domain.
   * The propery has to be a vtkSMStringVectorProperty. If all
   * vector values are in the domain, it returns 1. It returns
   * 0 otherwise.
   */
  virtual int IsInDomain(vtkSMProperty* property) VTK_OVERRIDE;

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
  virtual void Update(vtkSMProperty*) VTK_OVERRIDE;

  /**
   * Set the value of an element of a property from the animation editor.
   */
  virtual void SetAnimationValue(vtkSMProperty*, int, double) VTK_OVERRIDE;

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
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMStringListDomain();
  ~vtkSMStringListDomain();
  //@}

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  virtual void ChildSaveState(vtkPVXMLElement* domainElement) VTK_OVERRIDE;

  //@{
  /**
   * Call to set the strings. Will fire DomainModifiedEvent if the domain values
   * have indeed changed.
   */
  virtual void SetStrings(const std::vector<vtkStdString>& strings);
  const std::vector<vtkStdString>& GetStrings();
  //@}

private:
  vtkSMStringListDomain(const vtkSMStringListDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMStringListDomain&) VTK_DELETE_FUNCTION;

  vtkSMStringListDomainInternals* SLInternals;
};

#endif
