// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqViewTypePropertyWidget_h
#define pqViewTypePropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QComboBox;

/**
 * pqViewTypePropertyWidget is a property widget that shows a combo-box with
 * values equal to the currently available types of views. This could have been
 * implemented as a domain, but I was being lazy :).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewTypePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QString value READ value WRITE setValue);

public:
  pqViewTypePropertyWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqViewTypePropertyWidget() override;

  /**
   * get the current value in the widget
   */
  QString value() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * set the value for the widget.
   */
  void setValue(const QString& value);

Q_SIGNALS:
  void valueChanged();

private:
  Q_DISABLE_COPY(pqViewTypePropertyWidget)
  QPointer<QComboBox> ComboBox;
};

#endif
