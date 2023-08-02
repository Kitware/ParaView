// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDomainIterator
 * @brief   iterates over domains of a property
 *
 * vtkSMDomainIterator iterates over the domains of a property.
 */

#ifndef vtkSMDomainIterator_h
#define vtkSMDomainIterator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMDomain;

struct vtkSMDomainIteratorInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDomainIterator : public vtkSMObject
{
public:
  static vtkSMDomainIterator* New();
  vtkTypeMacro(vtkSMDomainIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A property must be assigned before iteration is performed.
   */
  void SetProperty(vtkSMProperty* property);

  ///@{
  /**
   * Returns the property being iterated over.
   */
  vtkGetObjectMacro(Property, vtkSMProperty);
  ///@}

  /**
   * Go to the first domain.
   */
  void Begin();

  /**
   * Is the iterator at the end of the list.
   */
  int IsAtEnd();

  /**
   * Move to the next iterator.
   */
  void Next();

  /**
   * Returns the key (the name) of the current domain.
   */
  const char* GetKey();

  /**
   * Returns the current domain.
   */
  vtkSMDomain* GetDomain();

protected:
  vtkSMDomainIterator();
  ~vtkSMDomainIterator() override;

  vtkSMProperty* Property;

private:
  vtkSMDomainIteratorInternals* Internals;

  vtkSMDomainIterator(const vtkSMDomainIterator&) = delete;
  void operator=(const vtkSMDomainIterator&) = delete;
};

#endif
