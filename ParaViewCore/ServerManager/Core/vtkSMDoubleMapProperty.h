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
 * @class   vtkSMDoubleMapProperty
 * @brief   a map property storing double values
 *
 * vtkSMDoubleMapProperty stores a map of vtkIdType keys to double values.
*/

#ifndef vtkSMDoubleMapProperty_h
#define vtkSMDoubleMapProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMMapProperty.h"

class vtkSMDoubleMapPropertyPrivate;
class vtkSMDoubleMapPropertyIterator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDoubleMapProperty : public vtkSMMapProperty
{
public:
  static vtkSMDoubleMapProperty* New();
  vtkTypeMacro(vtkSMDoubleMapProperty, vtkSMMapProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Sets the number of components.
   */
  void SetNumberOfComponents(unsigned int components);

  /**
   * Returns the number of components.
   */
  unsigned int GetNumberOfComponents();

  /**
   * Sets the element at index to value.
   */
  void SetElement(vtkIdType index, double value);

  /**
   * Sets the elements at index to values.
   */
  void SetElements(vtkIdType index, const double* values);

  /**
   * Sets the elements at index to values.
   */
  void SetElements(vtkIdType index, const double* values, unsigned int numValues);

  /**
   * Sets the component at index to value.
   */
  void SetElementComponent(vtkIdType index, unsigned int component, double value);

  /**
   * Returns the element at index.
   */
  double GetElement(vtkIdType index);

  /**
   * Returns the elements at index.
   */
  double* GetElements(vtkIdType index);

  /**
   * Returns the element component at index.
   */
  double GetElementComponent(vtkIdType index, vtkIdType component);

  /**
   * Removes the element at index.
   */
  void RemoveElement(vtkIdType index);

  /**
   * Returns the number of elements.
   */
  virtual vtkIdType GetNumberOfElements() VTK_OVERRIDE;

  /**
   * Returns true if the property has an element with the given index
   */
  bool HasElement(vtkIdType index);

  /**
   * Clears all of the elements from the property.
   */
  void ClearElements();

  /**
   * Returns a new iterator for the map.
   */
  VTK_NEWINSTANCE
  vtkSMDoubleMapPropertyIterator* NewIterator();

  void* GetMapPointer();

  /**
   * Copy all property values.
   */
  virtual void Copy(vtkSMProperty* src) VTK_OVERRIDE;

  /**
   * For properties that support specifying defaults in XML configuration, this
   * method will reset the property value to the default values specified in the
   * XML.
   */
  virtual void ResetToXMLDefaults() VTK_OVERRIDE;

protected:
  vtkSMDoubleMapProperty();
  ~vtkSMDoubleMapProperty();

  virtual void WriteTo(vtkSMMessage* msg) VTK_OVERRIDE;

  virtual void ReadFrom(
    const vtkSMMessage* message, int message_offset, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  virtual int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) VTK_OVERRIDE;

  virtual void SaveStateValues(vtkPVXMLElement* propertyElement) VTK_OVERRIDE;
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader) VTK_OVERRIDE;

private:
  vtkSMDoubleMapProperty(const vtkSMDoubleMapProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMDoubleMapProperty&) VTK_DELETE_FUNCTION;

  vtkSMDoubleMapPropertyPrivate* Private;
};

#endif // vtkSMDoubleMapProperty_h
