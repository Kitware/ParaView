// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqCollapsedGroup_h
#define pqCollapsedGroup_h

#include "pqWidgetsModule.h"
#include <QGroupBox>

class PQWIDGETS_EXPORT pqCollapsedGroup : public QGroupBox
{
  Q_OBJECT
  Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed)
public:
  explicit pqCollapsedGroup(QWidget* p = nullptr);

  bool collapsed() const;
  void setCollapsed(bool);

  QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void childEvent(QChildEvent* c) override;

  virtual void setChildrenEnabled(bool);

  bool Collapsed;
  bool Pressed;

private:
  static QStyleOptionGroupBox pqCollapseGroupGetStyleOption(const pqCollapsedGroup* p);
  QRect textRect();
  QRect collapseRect();
};

#endif // pqCollapsedGroup_h
