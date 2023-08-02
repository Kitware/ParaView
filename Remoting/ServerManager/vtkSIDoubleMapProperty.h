// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIDoubleMapProperty
 *
 * Map property that manage double value to be set through a method
 * on a vtkObject.
 */

#ifndef vtkSIDoubleMapProperty_h
#define vtkSIDoubleMapProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDoubleMapProperty : public vtkSIProperty
{
public:
  static vtkSIDoubleMapProperty* New();
  vtkTypeMacro(vtkSIDoubleMapProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(CleanCommand);
  vtkSetStringMacro(CleanCommand);

protected:
  vtkSIDoubleMapProperty();
  ~vtkSIDoubleMapProperty() override;

  bool Push(vtkSMMessage*, int) override;
  bool ReadXMLAttributes(vtkSIProxy* parent, vtkPVXMLElement* element) override;

  unsigned int NumberOfComponents;
  char* CleanCommand;

private:
  vtkSIDoubleMapProperty(const vtkSIDoubleMapProperty&) = delete;
  void operator=(const vtkSIDoubleMapProperty&) = delete;
};

#endif
