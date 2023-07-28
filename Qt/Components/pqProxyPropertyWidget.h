// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqProxyPropertyWidget_h
#define pqProxyPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QPointer>

class pqSelectionInputWidget;
class pqProxySelectionWidget;
class vtkSMProxy;

/**
 * This is a widget for a vtkSMProxyProperty. It handles a "SelectionInput"
 * property and properties with ProxyListDomain.
 */
class PQCOMPONENTS_EXPORT pqProxyPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqProxyPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);

  /**
   * Overridden to pass the calls to internal widgets.
   */
  void apply() override;
  void reset() override;

  /**
   * These methods are called when the pqProxyPropertiesPanel containing the
   * widget is activated/deactivated. Only widgets that have 3D widgets need to
   * override these methods to select/deselect the 3D widgets.
   */
  void select() override;
  void deselect() override;

  /**
   * Overridden to hide the properties for proxies in a vtkSMProxyListDomain if
   * requested.
   */
  void updateWidget(bool showing_advanced_properties) override;

  /**
   * If the internal widget is a ProxySelectionWidget, return its chosen proxy. Otherwise null
   */
  vtkSMProxy* chosenProxy() const;

private:
  QPointer<pqSelectionInputWidget> SelectionInputWidget;
  QPointer<pqProxySelectionWidget> ProxySelectionWidget;
};

#endif // pqProxyPropertyWidget_h
