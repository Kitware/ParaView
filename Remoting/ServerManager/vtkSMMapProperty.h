// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMMapProperty
 * @brief   abstract superclass for all map properties
 *
 * vtkSMMapProperty defines an interface common for all map properties.
 * A map property stores a set of keys and values.
 */

#ifndef vtkSMMapProperty_h
#define vtkSMMapProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMMapProperty : public vtkSMProperty
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
