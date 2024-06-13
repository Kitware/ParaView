// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqBlockProxyWidget_h
#define pqBlockProxyWidget_h

#include "pqComponentsModule.h"
#include "pqProxyWidget.h"

#include <QString>

class PQCOMPONENTS_EXPORT pqBlockProxyWidget : public pqProxyWidget
{
  Q_OBJECT
  typedef pqProxyWidget Superclass;

public:
  pqBlockProxyWidget(vtkSMProxy* proxy, QString selector, QWidget* parent,
    Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqBlockProxyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void apply() const override;

protected Q_SLOTS:
  void onChangeFinished() override;

private:
  QString Selector;
};

#endif // pqBlockProxyWidget_h
