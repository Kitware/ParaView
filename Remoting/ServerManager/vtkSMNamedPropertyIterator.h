/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedPropertyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMNamedPropertyIterator
 * @brief   iterates over a subset of a proxy's properties
 *
 *
 * vtkSMNamedPropertyIterator can be used to iterate over a subset of a proxy's
 * properties. The subset is defined through a list of strings naming properties.
 * The properties of the root proxies as well as sub-proxies are  included in the
 * iteration. For sub-proxies, only exposed properties are iterated over.
 *
 * @sa
 * vtkSMPropertyIterator
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
*/

#ifndef vtkSMNamedPropertyIterator_h
#define vtkSMNamedPropertyIterator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMPropertyIterator.h"

class vtkSMProperty;
class vtkSMProxy;
class vtkStringList;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMNamedPropertyIterator : public vtkSMPropertyIterator
{
public:
  static vtkSMNamedPropertyIterator* New();
  vtkTypeMacro(vtkSMNamedPropertyIterator, vtkSMPropertyIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the names of properties to iterate over.
   */
  void SetPropertyNames(vtkStringList* names);

  /**
   * Go to the first property.
   */
  void Begin() override;

  /**
   * Returns true if iterator points past the end of the collection.
   */
  int IsAtEnd() override;

  /**
   * Move to the next property.
   */
  void Next() override;

  /**
   * Returns the key (name) at the current iterator position.
   */
  const char* GetKey() override;

  /**
   * Returns the XMLLabel for self properties and the exposed name for
   * sub-proxy properties.
   */
  const char* GetPropertyLabel() override;

  /**
   * Returns the property at the current iterator position.
   */
  vtkSMProperty* GetProperty() override;

protected:
  vtkSMNamedPropertyIterator();
  ~vtkSMNamedPropertyIterator() override;

  vtkStringList* PropertyNames;
  int PropertyNameIndex;

private:
  vtkSMNamedPropertyIterator(const vtkSMNamedPropertyIterator&) = delete;
  void operator=(const vtkSMNamedPropertyIterator&) = delete;
};

#endif
