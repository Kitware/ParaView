/*=========================================================================

   Program: ParaView
   Module:  pqCheckableProperty.h

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
#ifndef pqCheckableProperty_h
#define pqCheckableProperty_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <memory>

/**
 * @class pqCheckableProperty
 * @brief property widget for properties representing a checkable property.
 *
 * pqCheckableProperty is designed for a specific use-case: a property that can be enabled
 * or disabled by a checkbox. This is useful for optional properties since it restrains the
 * user to use the property unless it is really needed.
 *
 * Example proxy definition(s) that use this widget is as follows:
 *
 * @code{xml}
 *     <ServerManagerConfiguration>
 *       <ProxyGroup name="sources">
 *         <SourceProxy name="CustomSource" class="vtkSphereSource">
 *           <DoubleVectorProperty name="PropertyA"
 *                              number_of_elements="1"
 *                              default_values="0">
 *             <DoubleRangeDomain name="range" min="0" max="10" />
 *           </DoubleVectorProperty>
 *
 *           <IntVectorProperty name="EnablePropertyA"
 *                              number_of_elements="1"
 *                              default_values="0">
 *             <BooleanDomain name="bool" />
 *           </IntVectorProperty>
 *
 *           <PropertyGroup label="Property A" panel_widget="CheckableProperty">
 *             <Property name="PropertyA" function="Property" />
 *             <Property name="EnablePropertyA" function="PropertyCheckBox" />
 *           </PropertyGroup>
 *         </SourceProxy>
 *       </ProxyGroup>
 *     </ServerManagerConfiguration>
 * @endcode
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCheckableProperty : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqCheckableProperty(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqCheckableProperty() override;

  void apply() override;
  void reset() override;
  void select() override;
  void deselect() override;
  void updateWidget(bool showing_advanced_properties) override;
  void setPanelVisibility(const char* vis) override;
  void setView(pqView*) override;
  bool isSingleRowItem() const override;

  bool enableCheckbox() const;

public Q_SLOTS:
  void setEnableCheckbox(bool enableCheckbox);

private:
  Q_PROPERTY(bool enableCheckbox READ enableCheckbox WRITE setEnableCheckbox)

  struct Internal;
  std::unique_ptr<Internal> internal;
};

#endif
