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
// .NAME vtkSMDoubleMapProperty - a map property storing double values
// .SECTION Description
// vtkSMDoubleMapProperty stores a map of vtkIdType keys to double values.

#ifndef __vtkSMDoubleMapProperty_h
#define __vtkSMDoubleMapProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMMapProperty.h"

class vtkSMDoubleMapPropertyPrivate;
class vtkSMDoubleMapPropertyIterator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDoubleMapProperty : public vtkSMMapProperty
{
public:
  static vtkSMDoubleMapProperty* New();
  vtkTypeMacro(vtkSMDoubleMapProperty, vtkSMMapProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets the number of components.
  void SetNumberOfComponents(unsigned int components);

  // Description:
  // Returns the number of components.
  unsigned int GetNumberOfComponents();

  // Description:
  // Sets the element at index to value.
  void SetElement(vtkIdType index, double value);

  // Description:
  // Sets the elements at index to values.
  void SetElements(vtkIdType index, const double *values);

  // Description:
  // Sets the elements at index to values.
  void SetElements(vtkIdType index, const double *values, unsigned int numValues);

  // Description:
  // Sets the component at index to value.
  void SetElementComponent(vtkIdType index, unsigned int component, double value);

  // Description:
  // Returns the element at index.
  double GetElement(vtkIdType index);

  // Description:
  // Returns the elements at index.
  double* GetElements(vtkIdType index);

  // Description:
  // Returns the element component at index.
  double GetElementComponent(vtkIdType index, vtkIdType component);

  // Description:
  // Removes the element at index.
  void RemoveElement(vtkIdType index);

  // Description:
  // Returns the number of elements.
  virtual vtkIdType GetNumberOfElements();

  // Description:
  // Clears all of the elements from the property.
  void ClearElements();

  // Description:
  // Returns a new iterator for the map.
  vtkSMDoubleMapPropertyIterator* NewIterator();

  void* GetMapPointer();

  // Description:
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

  // Description:
  // For properties that support specifying defaults in XML configuration, this
  // method will reset the property value to the default values specified in the
  // XML.
  virtual void ResetToXMLDefaults();

protected:
  vtkSMDoubleMapProperty();
  ~vtkSMDoubleMapProperty();

  virtual void WriteTo(vtkSMMessage* msg);

  virtual void ReadFrom(const vtkSMMessage *message,
                        int message_offset,
                        vtkSMProxyLocator* locator);

  virtual int ReadXMLAttributes(vtkSMProxy* parent,
                                vtkPVXMLElement* element);

  virtual void SaveStateValues(vtkPVXMLElement* propertyElement);
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);

private:
  vtkSMDoubleMapProperty(const vtkSMDoubleMapProperty&); // Not implemented
  void operator=(const vtkSMDoubleMapProperty&); // Not implemented

  vtkSMDoubleMapPropertyPrivate *Private;
};

#endif // __vtkSMDoubleMapProperty_h
