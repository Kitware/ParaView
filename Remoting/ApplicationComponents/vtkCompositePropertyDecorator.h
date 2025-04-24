// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCompositePropertyDecorator_h
#define vtkCompositePropertyDecorator_h

#include "vtkPropertyDecorator.h"
#include "vtkRemotingApplicationComponentsModule.h"

#include <memory> // for std::unique_ptr

/**
 * @class vtkCompositePropertyDecorator
 * @brief vtkPropertyDecorator subclass that can combine multiple
 *        decorators using boolean operations.
 *
 * vtkCompositePropertyDecorator helps combine multiple decorators using
 * boolean operators as indicated in the example below.
 *
 * @code{xml}
 * <IntVectorProperty name="Expression1"
 *    number_of_elements="1"
 *    default_values="0">
 *    <BooleanDomain name="bool" />
 *    <Hints>
 *      <!-- Expression === { A and [B or (C and D)] } or E -->
 *      <PropertyWidgetDecorator type="CompositeDecorator">
 *        <Expression type="or">
 *          <Expression type="and">
 *            <PropertyWidgetDecorator type="GenericDecorator" mode="enabled_state" property="A"
 *                  value="1" />
 *            <Expression type="or">
 *              <PropertyWidgetDecorator type="GenericDecorator" mode="enabled_state" property="B"
 *                  value="1" />
 *              <Expression type="and">
 *                <PropertyWidgetDecorator type="GenericDecorator" mode="enabled_state" property="C"
 *                    value="1" />
 *                <PropertyWidgetDecorator type="GenericDecorator" mode="enabled_state" property="D"
 *                    value="1" />
 *              </Expression>
 *            </Expression>
 *          </Expression>
 *          <PropertyWidgetDecorator type="GenericDecorator" mode="enabled_state" property="E"
 *              value="1" />
 *        </Expression>
 *      </PropertyWidgetDecorator>
 *    </Hints>
 * </IntVectorProperty>
 * @endcode
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkCompositePropertyDecorator
  : public vtkPropertyDecorator
{
public:
  static vtkCompositePropertyDecorator* New();
  vtkTypeMacro(vtkCompositePropertyDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy) override;

  bool CanShow(bool show_advanced) const override;
  bool Enable() const override;

  /*
   * @brief Register a new decorator
   *
   * When vtkCompositePropertyDecorator iterates its Expresssion it tries to
   * create a vtkPropertyDecorator  of "type" as described in its XML hint.  By
   * defasult it calls vtkPropertyDecorator::Create . If that function returns
   * nullptr it will try to use any of the functions registered via
   * RegisterDecorator.  This ability allows us to handle new types of
   * decorators dynamically which can appear via plugins.  Currently this
   * ability is used by pqCompositePropertyWidgetDecorator which is the comsumer
   * of this class
   * @{{
   */
  using DecoratorCreationFunction =
    std::function<vtkSmartPointer<vtkPropertyDecorator>(vtkPVXMLElement*, vtkSMProxy*)>;
  void RegisterDecorator(DecoratorCreationFunction func);
  /*
   * @}}
   */
protected:
  vtkCompositePropertyDecorator();
  ~vtkCompositePropertyDecorator() override;

private:
  vtkCompositePropertyDecorator(const vtkCompositePropertyDecorator&) = delete;
  void operator=(const vtkCompositePropertyDecorator&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
