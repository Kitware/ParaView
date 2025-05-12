// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkInputDataTypeDecorator_h
#define vtkInputDataTypeDecorator_h
#include "vtkPropertyDecorator.h"
#include "vtkRemotingApplicationComponentsModule.h"
#include "vtkWeakPointer.h"

class vtkSMProxy;

/**
 * vtkInputDataTypeDecorator is a vtkPropertyWidgetDecorator subclass.
 * For certain properties, they should update the enable state
 * based on input data types.
 * For example, "Computer Gradients" in Contour filter should only
 * be enabled when an input data type is a StructuredData. Please see
 * vtkPVDataInformation::IsDataStructured() for structured types.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkInputDataTypeDecorator
  : public vtkPropertyDecorator
{

public:
  static vtkInputDataTypeDecorator* New();
  vtkTypeMacro(vtkInputDataTypeDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy) override;

  /**
   * Overridden to enable/disable the widget based on input data type.
   */
  bool Enable() const override;

  /**
   * Overriden to show or not the widget based on input data type.
   */
  bool CanShow(bool show_advanced) const override;

protected:
  vtkInputDataTypeDecorator();
  ~vtkInputDataTypeDecorator() override;

  virtual bool ProcessState() const;

private:
  vtkInputDataTypeDecorator(const vtkInputDataTypeDecorator&) = delete;
  void operator=(const vtkInputDataTypeDecorator&) = delete;

  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverId;
};

#endif
