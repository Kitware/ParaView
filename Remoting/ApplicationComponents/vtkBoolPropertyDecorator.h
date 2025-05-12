// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkBoolPropertyDecorator_h
#define vtkBoolPropertyDecorator_h

#include "vtkRemotingApplicationComponentsModule.h"

#include "vtkPropertyDecorator.h"
#include "vtkWeakPointer.h"

class vtkSMProperty;
class pqBoolPropertyWidgetDecorator;

/**
 * vtkBoolPropertyDecorator is a base class for enable/disable
 * or show/hide widgets based on the status of another property not
 * directly controlled by the widget.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkBoolPropertyDecorator : public vtkPropertyDecorator
{

public:
  static vtkBoolPropertyDecorator* New();
  vtkTypeMacro(vtkBoolPropertyDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

public:
  void Initialize(vtkPVXMLElement* xml, vtkSMProxy* proxy) override;

  bool IsBoolProperty() const { return this->BoolProperty; }

  enum
  {
    BoolPropertyChangedEvent = vtkCommand::UserEvent + 1002,
  };

protected:
  vtkBoolPropertyDecorator();
  ~vtkBoolPropertyDecorator() override;

private:
  vtkBoolPropertyDecorator(const vtkBoolPropertyDecorator&) = delete;
  void operator=(const vtkBoolPropertyDecorator&) = delete;

  void SetBoolProperty(bool);

  friend class pqBoolPropertyWidgetDecorator;
  void UpdateBoolPropertyState();

  vtkWeakPointer<vtkSMProperty> Property;
  std::string Function;
  int Index;
  unsigned long ObserverId;
  bool BoolProperty;
  std::string Value;
};

#endif
