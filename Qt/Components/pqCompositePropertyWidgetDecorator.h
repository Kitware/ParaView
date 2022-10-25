/*=========================================================================

   Program: ParaView
   Module:  pqCompositePropertyWidgetDecorator.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
