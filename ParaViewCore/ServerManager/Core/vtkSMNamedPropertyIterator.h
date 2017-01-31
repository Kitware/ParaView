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

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMPropertyIterator.h"

class vtkSMProperty;
class vtkSMProxy;
class vtkStringList;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMNamedPropertyIterator : public vtkSMPropertyIterator
{
public:
  static vtkSMNamedPropertyIterator* New();
  vtkTypeMacro(vtkSMNamedPropertyIterator, vtkSMPropertyIterator);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the names of properties to iterate over.
   */
  void SetPropertyNames(vtkStringList* names);

  /**
   * Go to the first property.
   */
  virtual void Begin() VTK_OVERRIDE;

  /**
   * Returns true if iterator points past the end of the collection.
   */
  virtual int IsAtEnd() VTK_OVERRIDE;

  /**
   * Move to the next property.
   */
  virtual void Next() VTK_OVERRIDE;

  /**
   * Returns the key (name) at the current iterator position.
   */
  virtual const char* GetKey() VTK_OVERRIDE;

  /**
   * Returns the XMLLabel for self properties and the exposed name for
   * sub-proxy properties.
   */
  virtual const char* GetPropertyLabel() VTK_OVERRIDE;

  /**
   * Returns the property at the current iterator position.
   */
  virtual vtkSMProperty* GetProperty() VTK_OVERRIDE;

protected:
  vtkSMNamedPropertyIterator();
  ~vtkSMNamedPropertyIterator();

  vtkStringList* PropertyNames;
  int PropertyNameIndex;

private:
  vtkSMNamedPropertyIterator(const vtkSMNamedPropertyIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMNamedPropertyIterator&) VTK_DELETE_FUNCTION;
};

#endif
