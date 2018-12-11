/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMapProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMMapProperty
 * @brief   abstract superclass for all map properties
 *
 * vtkSMMapProperty defines an interface common for all map properties.
 * A map property stores a set of keys and values.
*/

#ifndef vtkSMMapProperty_h
#define vtkSMMapProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProperty.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMMapProperty : public vtkSMProperty
{
public:
  vtkTypeMacro(vtkSMMapProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the number of elements for the value type.
   */
  virtual vtkIdType GetNumberOfElements();

  /**
   * Returns true if the current value is the same as the default value.
   */
  bool IsValueDefault() override;

  /**
   * Copy all property values.
   */
  void Copy(vtkSMProperty* src) override;

protected:
  vtkSMMapProperty();
  ~vtkSMMapProperty() override;

  int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader) override;
  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;

private:
  vtkSMMapProperty(const vtkSMMapProperty&) = delete;
  void operator=(const vtkSMMapProperty&) = delete;
};

#endif // vtkSMMapProperty_h
