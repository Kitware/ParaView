// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqArrayListWidget_h
#define pqArrayListWidget_h

#include "pqComponentsModule.h"

#include <QWidget>

class pqArrayListModel;
class pqExpandableTableView;

/**
 * @class pqArrayListWidget
 * @brief A widget for array labeling.
 *
 * pqArrayListWidget is intended to be used with 2-elements string properties.
 * It creates a two-columns string table.
 * First column is readonly, the second is editable.
 *
 * It is useful to associate an editable label to each array,
 * for array renaming for instance.
 */
class PQCOMPONENTS_EXPORT pqArrayListWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;

public:
  pqArrayListWidget(QWidget* parent = nullptr);
  ~pqArrayListWidget() override = default;

  /**
   * overridden to handle QDynamicPropertyChangeEvent events.
   */
  bool event(QEvent* e) override;

  /**
   * Set table header label. It should match the Property name.
   */
  void setHeaderLabel(const QString& label);

  /**
   * Set the maximum number of visible rows. Beyond this threshold,
   * a scrollbar is added.
   * Forwarded to the inner pqTableView.
   */
  void setMaximumRowCountBeforeScrolling(int size);

  /**
   * Set the icon type corresponding to the arrays.
   */
  void setIconType(const QString& icon_type);

Q_SIGNALS:
  /**
   * fired whenever the state has been modified.
   */
  void widgetModified();

protected:
  /**
   * called in `event()` to handle change in a dynamic property with the given name.
   */
  void propertyChanged(const QString& pname);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateProperty();

private:
  Q_DISABLE_COPY(pqArrayListWidget);

  pqArrayListModel* Model = nullptr;
  pqExpandableTableView* TableView = nullptr;

  bool UpdatingProperty = false;
};

#endif
