// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDynamicPropertiesWidget_h
#define pqDynamicPropertiesWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QMap>
#include <QString>

class QFormLayout;
class QGroupBox;
class QVBoxLayout;

/**
 *
 *
 * @class   pqDynamicPropertiesWidget
 * @brief   Creates a panel_widget for setting dynamically generated properties
 *
 * The properties have a name, a type, a default value and optionally
 * a min and a max value. We can have bool, int and double properties.
 *
 * See:
 * - vtkPVRenderView::GetANARIRendererParameters for the json
 * description of properties associated with an ANARI renderer
 *
 * - vtkDynamicProperties for the keys and types that can be used in
 * the json file.
 *
 * - ANARIRenderParameter XML property definiton for the RenderViewProxy
 * in view_removingviews.xml
 *
 * - pqIndexSelectionWidget for a similar dynamic widget that works only for
 * int properties.
 */
class PQCOMPONENTS_EXPORT pqDynamicPropertiesWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqDynamicPropertiesWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqDynamicPropertiesWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setHeaderLabel(const QString& str);
  void setPushPropertyName(const QByteArray& pName);

Q_SIGNALS:
  void widgetModified();

protected Q_SLOTS:
  /**
   * Start a timer that calls updatePropertyImpl. This slot is triggered when
   * a widget is modified.
   */
  void updateProperty();
  /**
   * Update the Qt property with current widget state. Emits widgetModified.
   */
  void updatePropertyImpl();

protected: // NOLINT(readability-redundant-access-specifiers)
  bool eventFilter(QObject* obj, QEvent* e) override;
  /**
   * Update the widget state from the PropertyLink Qt property.
   */
  void propertyChanged();

private:
  Q_DISABLE_COPY(pqDynamicPropertiesWidget)

  void buildWidget(vtkSMProperty* infoProp);
  void clearProperties();
  int findRow(const QString& key);

  /**
   * The names of the Qt properties used to sync with the vtkSMProperty.
   */
  QByteArray PushPropertyName;

  bool PropertyUpdatePending;     // Only update the property once per 250 ms.
  bool IgnorePushPropertyUpdates; // don't react to our own updates.

  QVBoxLayout* VBox;
  QGroupBox* GroupBox;
  QFormLayout* Form;

  class pqInternals;
  pqInternals* Internals;

  class DomainModifiedObserver;
  friend DomainModifiedObserver;
};

#endif // pqDynamicPropertiesWidget_h
