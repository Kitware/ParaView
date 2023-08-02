// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyEditorPropertyWidget_h
#define pqProxyEditorPropertyWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"

class pqProxyWidgetDialog;
class QCheckBox;
class QPushButton;

/**
 * Property widget that can be used to edit a proxy set as the value of a
 * ProxyProperty in a pop-up dialog. e.g. RenderView proxy has a "GridActor3D"
 * ProxyProperty which is set to a "GridActor3D" proxy. Using this widget
 * allows the users to edit the properties on this GridActor3D, when present.
 *
 * To use this widget, add 'panel_widget="proxy_editor"' to the property's XML.
 *
 * Since it's common to have a property on the proxy being edited in the popup
 * dialog which controls its visibility (or enabled state), we add support for
 * putting that property as a checkbox in this pqProxyEditorPropertyWidget
 * itself. For that, one can use `ProxyEditorPropertyWidget` hints.
 *
 * Example XML:
 * @code
 *   <ProxyProperty name="AxesGrid" ...  >
 *     <ProxyListDomain name="proxy_list">
 *       <Proxy group="annotations" name="GridAxes3DActor" />
 *     </ProxyListDomain>
 *     <Hints>
 *       <ProxyEditorPropertyWidget property="Visibility" />
 *     </Hints>
 *   </ProxyProperty>
 * @endcode
 */
class PQCOMPONENTS_EXPORT pqProxyEditorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy proxyToEdit READ proxyToEdit WRITE setProxyToEdit)

public:
  pqProxyEditorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqProxyEditorPropertyWidget() override;

  pqSMProxy proxyToEdit() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setProxyToEdit(pqSMProxy);

protected Q_SLOTS:
  void buttonClicked();

Q_SIGNALS:
  void dummySignal();

private:
  Q_DISABLE_COPY(pqProxyEditorPropertyWidget)

  vtkWeakPointer<vtkSMProxy> ProxyToEdit;
  QPointer<QPushButton> Button;
  QPointer<QCheckBox> Checkbox;
  QPointer<pqProxyWidgetDialog> Editor;
  QString PropertyName;
};

#endif
