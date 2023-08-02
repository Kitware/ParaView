// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTreeWidgetItemObject_h
#define pqTreeWidgetItemObject_h

#include "pqWidgetsModule.h"
#include <QObject>
#include <QTreeWidgetItem>

/**
 * QTreeWidgetItem subclass with additional signals, slots, and properties
 */
class PQWIDGETS_EXPORT pqTreeWidgetItemObject
  : public QObject
  , public QTreeWidgetItem
{
  Q_OBJECT
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
public:
  /**
   * construct list widget item to for QTreeWidget with a string
   */
  pqTreeWidgetItemObject(const QStringList& t, int type = QTreeWidgetItem::UserType);
  pqTreeWidgetItemObject(
    QTreeWidget* p, const QStringList& t, int type = QTreeWidgetItem::UserType);
  pqTreeWidgetItemObject(
    QTreeWidgetItem* p, const QStringList& t, int type = QTreeWidgetItem::UserType);

  /**
   * overload setData() to emit changed signal
   */
  void setData(int column, int role, const QVariant& v) override;

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

  /**
   * Fired every time setData is called.
   */
  void modified();
};

#endif
