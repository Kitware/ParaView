// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkShowDecorator_h
#define vtkShowDecorator_h

#include "vtkRemotingApplicationComponentsModule.h"

#include "vtkBoolPropertyDecorator.h"
#include "vtkWeakPointer.h"

/**
 * vtkShowDecorator can be used to show/hide a widget based on the
 * status of another property not directly controlled by the widget.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkShowDecorator : public vtkBoolPropertyDecorator
{

public:
  static vtkShowDecorator* New();
  vtkTypeMacro(vtkShowDecorator, vtkBoolPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* xml, vtkSMProxy* proxy) override;

  bool CanShow(bool show_advanced) const override
  {
    (void)show_advanced;
    return this->IsBoolProperty();
  }

protected:
  vtkShowDecorator();
  ~vtkShowDecorator() override;

private:
  vtkShowDecorator(const vtkShowDecorator&) = delete;
  void operator=(const vtkShowDecorator&) = delete;
};

#endif
