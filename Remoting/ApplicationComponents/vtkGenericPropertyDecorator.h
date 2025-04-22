// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkGenericPropertyDecorator_h
#define vtkGenericPropertyDecorator_h

#include "vtkRemotingApplicationComponentsModule.h"

#include "vtkPropertyDecorator.h"

#include <memory>

/**
 * vtkGenericPropertyDecorator is a vtkPropertyDecorator that
 * supports multiple common use cases from a vtkPropertyDecorator.
 * The use cases supported are as follows:
 * \li 1. enabling the vtkPropertyWidget when the value of another
 *   property element matches a specific value (disabling otherwise).
 * \li 2. similar to 1, except instead of enabling/disabling the widget is made
 *   "default" when the values match and "advanced" otherwise.
 * \li 3. enabling the vtkPropertyWidget when the array named in the property
 *   has a specified number of components.
 * \li 4. as well as "inverse" of all the above i.e. when the value doesn't
 *   match the specified value.
 * Example usages:
 * \li VectorScaleMode, Stride, Seed, MaximumNumberOfSamplePoints properties on the Glyph proxy.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkGenericPropertyDecorator
  : public vtkPropertyDecorator
{
public:
  static vtkGenericPropertyDecorator* New();
  vtkTypeMacro(vtkGenericPropertyDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy) override;

  /**
   * Methods overridden from vtkPropertyDecorator.
   */
  bool CanShow(bool show_advanced) const override;
  bool Enable() const override;

  void UpdateState();

protected:
  vtkGenericPropertyDecorator();
  ~vtkGenericPropertyDecorator() override;

private:
  vtkGenericPropertyDecorator(const vtkGenericPropertyDecorator&) = delete;
  void operator=(const vtkGenericPropertyDecorator&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
