// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComboBoxStyle_h
#define pqComboBoxStyle_h

#include <QProxyStyle>

class pqComboBoxStyle : public QProxyStyle
{
  Q_OBJECT
public:
  explicit pqComboBoxStyle(bool showPopup);
  ~pqComboBoxStyle() override;

  int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
    QStyleHintReturn* ret) const override;

private:
  bool ShowPopup = false;
  Q_DISABLE_COPY(pqComboBoxStyle)
};
#endif
