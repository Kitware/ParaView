// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
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
 * The properties are specified using a JSON file.
 * The JSON contains a version and an array of parameters, and possible
 * additional key: value parameters which are ignored.
 *   each parameter has a name, type, description, default, min, max
 *     the type is an int given vtkDynamicProperties::Type (such as vtkDynamicProperties::Type)
 *     default, min and max have the specified type.
 * Example:
 * {
 *   "library": "environment"
 *   "renderer": "ao",
 *   "version": "1.0"
 *   "parameters":
 *   [
 *   {
 *     "name" : "sampleLimit",
 *     "type" :  <vtkDynamicProperties::Type>
 *     "default" : 128,
 *     "value": 0,
 *     "description" : "stop refining the frame after this number of samples",
 *     "max" : 100,
 *     "min" : 0,
 *   },
 *   ...
 *   ]
 * }
 * The key names and types are defined in vtkDynamicProperties class.
 * @see vtkDynamicProperties
 * The "library" and "renderer" key,values are for debugging and are
 * ignored by the dynamic properties panel.
 *
 * See:
 * - vtkPVRenderView::GetANARIRendererParameters for the JSON
 * description of properties associated with an ANARI renderer
 *
 * - vtkDynamicProperties for the keys and types that can be used in
 * the JSON file.
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
