// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCompositePropertyWidgetDecorator_h
#define pqCompositePropertyWidgetDecorator_h

#include "pqPropertyWidgetDecorator.h"

#include <QScopedPointer> // for QScopedPointer.

/**
 * @class pqCompositePropertyWidgetDecorator
 * @brief pqPropertyWidgetDecorator subclass that can combine multiple
 *        decorators using boolean operations.
 *
 * pqCompositePropertyWidgetDecorator helps combine multiple decorators using
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
class PQCOMPONENTS_EXPORT pqCompositePropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqCompositePropertyWidgetDecorator(vtkPVXMLElement* xml, pqPropertyWidget* parent);
  ~pqCompositePropertyWidgetDecorator() override;

  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;

private:
  Q_DISABLE_COPY(pqCompositePropertyWidgetDecorator)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
  friend class pqInternals;

  void handleNestedDecorator(pqPropertyWidgetDecorator*);
};

#endif
