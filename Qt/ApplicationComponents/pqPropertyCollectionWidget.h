/*=========================================================================

   Program: ParaView
   Module:  pqPropertyCollectionWidget.h

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
#ifndef pqPropertyCollectionWidget_h
#define pqPropertyCollectionWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * @class pqPropertyCollectionWidget
 * @brief property widget for properties representing collection of parameters.
 *
 * pqPropertyCollectionWidget is designed for a specific use-case: a group of properties on
 * a proxy all of which are repeatable vector properties (i.e. vtkSMVectorProperty
 * subclasses) where each repeatable item in the collection is to be edited together. Another way
 * of describing the same is that each vector property in the group defines a
 * column in a table with user having the ability to add and remove rows to this
 * table. However, instead of showing a literal tabular widget to edit values
 * in each row, a prototype proxy definition is provided using hints.
 *
 * Example proxy definition(s) that use this widget is as follows:
 *
 * @code{xml}
 *
 *  <SourceProxy class="vtkSteeringDataGenerator" name="Oscillators">
 *
 *    <DoubleVectorProperty name="Center"
 *                          command="SetTuple3Double"
 *                          use_index="1"
 *                          clean_command="Clear"
 *                          initial_string="coords"
 *                          number_of_elements_per_command="3"
 *                          repeat_command="1">
 *    </DoubleVectorProperty>
 *
 *    <IntVectorProperty name="Type"
 *                       command="SetTuple1Int"
 *                       clean_command="Clear"
 *                       use_index="1"
 *                       initial_string="type"
 *                       number_of_elements_per_command="1"
 *                       repeat_command="1">
 *    </IntVectorProperty>
 *
 *    <PropertyGroup label="Oscillators" panel_widget="PropertyCollection">
 *      <Property name="Center" function="PrototypeCenter" />
 *      <Property name="Type" function="PrototypeType" />
 *      <!-- here, "name" identifies the property on this proxy, while
 *           "function" identifies the property on the prototype proxy. If
 *           "function" is not specified, same value as "name" is assumed. -->
 *      <Hints>
 *        <PropertyCollectionWidgetPrototype group="misc" name="OscillatorPrototype" />
 *      </Hints>
 *    </PropertyGroup>
 *  </SourceProxy>
 *
 * @endcode
 *
 * The prototype proxy to use to build the UI for each row in this table that
 * has two columns for "Center" and "Type" is identified using the
 * `PropertyCollectionWidgetPrototype` hint added to the `PropertyGroup` element.
 * Following is an example definition:
 *
 * @code{xml}
 *  <ProxyGroup name="misc">
 *    <Proxy name="OscillatorPrototype" label="Oscillator" >
 *      <DoubleVectorProperty name="PrototypeCenter"
 *                            label="Center"
 *                            number_of_elements="3"
 *                            default_values="0 0 0">
 *        <DoubleRangeDomain name="range" />
 *        <Documentation>
 *          Specify center for the oscillator.
 *        </Documentation>
 *      </DoubleVectorProperty>
 *
 *      <IntVectorProperty name="PrototypeType"
 *                         label="Type"
 *                         number_of_elements="1"
 *                         default_values="0">
 *        <EnumerationDomain name="enum">
 *          <Entry text="damped" value="0" />
 *          <Entry text="decaying" value="1" />
 *          <Entry text="periodic" value="2" />
 *        </EnumerationDomain>
 *      </IntVectorProperty>
 *    </Proxy>
 *  </ProxyGroup>
 * @endcode
 *
 * A few things to note:
 * * Only vtkSMVectorProperty subclasses are supported and types must match
 *   between the property in the group and corresponding property on the
 *   prototype proxy.
 * * For each property in the group, the `number_of_elements_per_command`
 *   matches the `number_of_elements` on the corresponding property on the
 *   prototype proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPropertyCollectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqPropertyCollectionWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqPropertyCollectionWidget() override;

  /**
   * Overridden to handle QDynamicPropertyChangeEvent events.
   */
  bool event(QEvent* e) override;

Q_SIGNALS:
  void widgetModified();

private:
  Q_DISABLE_COPY(pqPropertyCollectionWidget);

  /**
   * called in `event()` to handle change in a dynamic property with the given name.
   */
  void propertyChanged(const char* pname);

  /**
   * called to updates the Qt dynamic properties on this QObject using the
   * current values on the UI. This is called as a consequence of some user
   * interaction to add/remove items to change their values.
   */
  void updateProperties();

  class pqInternals;
  pqInternals* Internals;
};

#endif
