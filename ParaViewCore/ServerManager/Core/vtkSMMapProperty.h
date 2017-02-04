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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the number of elements for the value type.
   */
  virtual vtkIdType GetNumberOfElements();

  /**
   * Returns true if the current value is the same as the default value.
   */
  virtual bool IsValueDefault() VTK_OVERRIDE;

  /**
   * Copy all property values.
   */
  virtual void Copy(vtkSMProperty* src) VTK_OVERRIDE;

protected:
  vtkSMMapProperty();
  ~vtkSMMapProperty();

  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader) VTK_OVERRIDE;
  virtual int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) VTK_OVERRIDE;

private:
  vtkSMMapProperty(const vtkSMMapProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMMapProperty&) VTK_DELETE_FUNCTION;
};

#endif // vtkSMMapProperty_h
