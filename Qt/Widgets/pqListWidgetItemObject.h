// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqListWidgetItemObject_h
#define pqListWidgetItemObject_h

#include "pqWidgetsModule.h"
#include <QListWidgetItem>
#include <QObject>

/**
 * QListWidgetItem subclass with additional signals, slots, and properties
 */
class PQWIDGETS_EXPORT pqListWidgetItemObject
  : public QObject
  , public QListWidgetItem
{
  Q_OBJECT
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
public:
  /**
   * construct list widget item to for QListWidget with a string
   */
  pqListWidgetItemObject(const QString& t, QListWidget* p);
  /**
   * overload setData() to emit changed signal
   */
  void setData(int role, const QVariant& v) override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * get the check true/false
   */
  bool isChecked() const;
  /**
   * set the check state true/false
   */
  void setChecked(bool v);

Q_SIGNALS:
  /**
   * signal check state changed
   */
  void checkedStateChanged(bool);
};

#endif
