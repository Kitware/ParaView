/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomainIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDomainIterator
 * @brief   iterates over domains of a property
 *
 * vtkSMDomainIterator iterates over the domains of a property.
*/

#ifndef vtkSMDomainIterator_h
#define vtkSMDomainIterator_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMDomain;

struct vtkSMDomainIteratorInternals;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDomainIterator : public vtkSMObject
{
public:
  static vtkSMDomainIterator* New();
  vtkTypeMacro(vtkSMDomainIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A property must be assigned before iteration is performed.
   */
  void SetProperty(vtkSMProperty* property);

  //@{
  /**
   * Returns the property being iterated over.
   */
  vtkGetObjectMacro(Property, vtkSMProperty);
  //@}

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
