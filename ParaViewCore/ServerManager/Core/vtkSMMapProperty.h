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
// .NAME vtkSMMapProperty - abstract superclass for all map properties
// .SECTION Description
// vtkSMMapProperty defines an interface common for all map properties.
// A map property stores a set of keys and values.

#ifndef __vtkSMMapProperty_h
#define __vtkSMMapProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProperty.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMMapProperty : public vtkSMProperty
{
public:
  vtkTypeMacro(vtkSMMapProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of elements for the value type.
  virtual vtkIdType GetNumberOfElements();

  // Description:
  // Returns true if the current value is the same as the default value.
  virtual bool IsValueDefault();

  // Description:
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

protected:
  vtkSMMapProperty();
  ~vtkSMMapProperty();

  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);
  virtual int ReadXMLAttributes(vtkSMProxy *parent, vtkPVXMLElement *element);

private:
  vtkSMMapProperty(const vtkSMMapProperty&); // Not implemented
  void operator=(const vtkSMMapProperty&); // Not implemented
};

#endif // __vtkSMMapProperty_h
