// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSignalAdaptorTreeWidget_h
#define pqSignalAdaptorTreeWidget_h

#include "pqComponentsModule.h"
#include <QList>
#include <QObject>
#include <QVariant>

class QTreeWidget;
class QTreeWidgetItem;

/**
 * pqSignalAdaptorTreeWidget can be used to connect any property with
 * repeat_command to a tree widget that displays the property value.
 * The TreeWidget must have exactly as many columns as the number of
 * elements in each command for the property
 * (i.e. number_of_element_per_command).
 * Note that the adaptor does not force the repeat command or
 * size requirements mentioned above.
 */
class PQCOMPONENTS_EXPORT pqSignalAdaptorTreeWidget : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues)

public:
  /**
   * Constructor.
   * \param treeWidget is the tree widget we are connecting.
   * \param editable indicates if items in the widget can be edited by
   * the user.
   */
  pqSignalAdaptorTreeWidget(QTreeWidget* treeWidget, bool editable);
  ~pqSignalAdaptorTreeWidget() override;

  /**
   * Returns a list of the values currently in the tree widget.
   */
  QList<QVariant> values() const;

  /**
   * Append an item to the tree.
   * The size of values == this->TreeWidget->columnCount().
   * Returns the newly created item, or 0 on failure.
   */
  QTreeWidgetItem* appendValue(const QList<QVariant>& values);
  QTreeWidgetItem* appendValue(const QStringList& values);

  /**
   * This adaptor create QTreeWidgetItem instances by default when new
   * entries are to be shown in the widget. To change the type of
   * QTreeWidgetItem subclass created, simply set a function pointer to
   * a callback which will be called every time a new item is needed.
   * The signature for the callback is:
   * QTreeWidgetItem* callback(QTreeWidget* parent, const QStringList& val)
   */
  void setItemCreatorFunction(QTreeWidgetItem*(fptr)(QTreeWidget*, const QStringList&))
  {
    this->ItemCreatorFunctionPtr = fptr;
  }
Q_SIGNALS:
  /**
   * Fired when the tree widget is modified.
   */
  void valuesChanged();

  /**
   * Fired when the table is automatically grown due to the user navigating
   * past the end. This only supported for editable pqTreeWidget instances.
   */
  void tableGrown(QTreeWidgetItem* item);

public Q_SLOTS:
  /**
   * Set the values in the widget.
   */
  void setValues(const QList<QVariant>&);

  /**
   * Called when user navigates beyond the end in the indices table widget. We
   * add a new row to simplify editing.
   */
  QTreeWidgetItem* growTable();

private Q_SLOTS:
  void sort(int);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSignalAdaptorTreeWidget)

  /**
   * Append an item to the tree.
   */
  void appendItem(QTreeWidgetItem* item);

  /**
   * Create a new QTreeWidgetItem instance.
   */
  QTreeWidgetItem* newItem(const QStringList& columnValues);

  void updateSortingLinks();

  QTreeWidget* TreeWidget;
  bool Editable;
  bool Sortable;
  QTreeWidgetItem* (*ItemCreatorFunctionPtr)(QTreeWidget*, const QStringList&);
};

#endif
