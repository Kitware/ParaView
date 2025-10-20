// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDoublePropertyMultiWidgets_h
#define pqDoublePropertyMultiWidgets_h

#include <pqPropertyWidget.h>

#include <QVariant> // for QVariantList

class pqDoubleRangeWidget;
class pqNumericParameter;
class vtkSMDoubleVectorProperty;

class QLabel;

/**
 * @brief pqDoublePropertyMultiWidgets is a property widgets that represents
 * a repeatable double property using one subwidget per value, typically a slider.
 *
 * It can not be used automatically from XML configuration because
 * each value has its own range, which cannot be described in XML.
 *
 * To use it, you need to create your own property widget.
 * For each value of the property, you should call addWidget().
 * When number of values changes, call clear() to remove all widgets.
 *
 * For instance, you may encapsulate a pqDoublePropertyMultiWidgets instance.
 * Then you should forward the changeAvailable() signal.
 */
class pqDoublePropertyMultiWidgets : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(QVariantList values READ values WRITE setValues);

public:
  pqDoublePropertyMultiWidgets(QWidget* parent, vtkSMDoubleVectorProperty* valuesProperty);
  ~pqDoublePropertyMultiWidgets() override = default;

  /**
   * Set the values on widgets.
   * Order matches `addWidget` calls.
   */
  void setValues(const QVariantList& values);

  /**
   * Return the list of values.
   * Order matches `addWidget` calls.
   */
  QVariantList values() const;

  /**
   * Remove all widgets.
   */
  void clear();

  /**
   * Append a new widget.
   */
  void addWidget(pqNumericParameter* param);

  /**
   * Hide the widgets for given parameter.
   * Name should match pqNumericParameter::getName()
   */
  void hideParameterWidgets(const QString& name);

Q_SIGNALS:
  /**
   * Emited on any underlying widget value change.
   */
  void valuesChanged();

private:
  QList<pqDoubleRangeWidget*> DynamicWidgets;
  QList<QLabel*> DynamicLabels;
};

#endif
