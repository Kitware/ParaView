// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxySelectionWidget_h
#define pqProxySelectionWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include <QScopedPointer>

class vtkSMProxy;
class vtkSMProperty;
class pqView;

/**
 * pqPropertyWidget that can be used for any proxy with a vtkSMProxyListDomain.
 * pqProxyPropertyWidget automatically creates this widget when it encounters a
 * property with vtkSMProxyListDomain.
 */
class PQCOMPONENTS_EXPORT pqProxySelectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy chosenProxy READ chosenProxy WRITE setChosenProxy)

public:
  /**
   * constructor requires the proxy, property. Note that this will abort if the
   * property does not have a ProxyListDomain.
   */
  pqProxySelectionWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqProxySelectionWidget() override;

  /**
   * get the selected proxy
   */
  vtkSMProxy* chosenProxy() const;
  void setChosenProxy(vtkSMProxy* proxy);

  /**
   * Overridden to forward the call to the internal pqProxyWidget maintained
   * for the chosen proxy.
   */
  void apply() override;
  void reset() override;
  void select() override;
  void deselect() override;
  void updateWidget(bool showing_advanced_properties) override;
  void setPanelVisibility(const char* vis) override;
  void setView(pqView*) override;

Q_SIGNALS:
  /**
   * Signal fired by setChosenProxy() when the proxy changes.
   */
  void chosenProxyChanged();

private Q_SLOTS:
  /**
   * Called when the current index in the combo-box is changed from the UI.
   * This calls setChosenProxy() with the argument as the proxy corresponding
   * to the \c idx from the domain.
   */
  void currentIndexChanged(int);

private: // NOLINT(readability-redundant-access-specifiers)
  class pqInternal;
  const QScopedPointer<pqInternal> Internal;

  Q_DISABLE_COPY(pqProxySelectionWidget)
};

#endif
