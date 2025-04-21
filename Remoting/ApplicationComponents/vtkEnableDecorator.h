// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEnableDecorator_h
#define vtkEnableDecorator_h

#include "vtkRemotingApplicationComponentsModule.h"

#include "vtkBoolPropertyDecorator.h"

/**
 * vtkEnableDecorator can be used to enable/disable a widget based on the
 * status of another property not directly controlled by the widget.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkEnableDecorator : public vtkBoolPropertyDecorator
{
public:
  static vtkEnableDecorator* New();
  vtkTypeMacro(vtkEnableDecorator, vtkBoolPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* xml, vtkSMProxy* proxy) override;

  /**
   * overridden from vtkProperty.
   */
  bool Enable() const override { return this->IsBoolProperty(); }

protected:
  vtkEnableDecorator();
  ~vtkEnableDecorator() override;

private:
  vtkEnableDecorator(const vtkEnableDecorator&) = delete;
  void operator=(const vtkEnableDecorator&) = delete;
};

#endif
