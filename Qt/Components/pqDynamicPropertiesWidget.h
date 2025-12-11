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
 * pqDynamicPropertiesWidget displays a list of labels and slider widgets,
 * intended to be used for selecting an index into a zero-based enumeration.
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
