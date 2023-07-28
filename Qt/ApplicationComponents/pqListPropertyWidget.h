// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqListPropertyWidget_h
#define pqListPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QVariant>

class QTableWidget;

/**
 * pqListPropertyWidget is a pqPropertyWidget that is used to show an editable
 * list of elements. This is suitable for int/idtype/double/string vector
 * properties with multiple elements, useful for allowing the user to change
 * a range of values without adding or removing entries from it.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqListPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> value READ value WRITE setValue NOTIFY valueChanged);

public:
  explicit pqListPropertyWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqListPropertyWidget() override;

  /**
   * Methods used to set/get the value for the widget.
   */
  QList<QVariant> value() const;
  void setValue(const QList<QVariant>& value);

Q_SIGNALS:
  void valueChanged();

private:
  Q_DISABLE_COPY(pqListPropertyWidget)
  QTableWidget* TableWidget;
};

#endif
