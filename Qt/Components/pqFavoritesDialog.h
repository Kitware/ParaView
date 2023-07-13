// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFavoritesDialog_h
#define pqFavoritesDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QPointer>

#include "vtkParaViewDeprecation.h" // for deprecation

class QVariant;
class QString;
class QTreeWidgetItem;

namespace Ui
{
class pqFavoritesDialog;
}

/**
 * pqFavoritesDialog is the Manage Favorites dialog used by ParaView.
 * It allows to create Favorites and organize them under custom categories.
 */
class PARAVIEW_DEPRECATED_IN_5_13_0(
  "Favorites should be replaced by Categories configuration. See pqConfigureCategories instead.")
  PQCOMPONENTS_EXPORT pqFavoritesDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqFavoritesDialog(const QVariant& filtersList, QWidget* p = nullptr);
  ~pqFavoritesDialog() override;

protected Q_SLOTS:
  /**
   * Create a category at the top level of the favorites tree.
   */
  void createCategory();

  /**
   * Add selected filters from the available filters tree to
   * the top level of favorites tree.
   */
  void onAddFavoritePressed();

  /**
   * Remove selected item (filters/categories) from favorites tree.
   * When removing a category, its children are removed too.
   */
  void onRemoveFavoritePressed();

  /**
   * Save current favorites tree and exit.
   */
  void onAccepted();

  /**
   * Ensure category name is unique when item text is changed.
   */
  void onItemChanged(QTreeWidgetItem*, int);

  /**
   * Update filters visibility according the search pattern.
   */
  void onSearchTextChanged(QString pattern);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Populate the favorites tree from the favorites settings.
   */
  void populateFavoritesTree();

  /**
   * Populate the filters tree from the favorites settings.
   */
  void populateFiltersTree(const QVariant& filtersList);

  /**
   * Recursively explore favorites.
   * Returned value is of the form  "proxyGroup;[parentCategories;...];displayName;[proxyName]| ..."
   * where :
   *  - proxyGroup is 'filters' or 'categories'
   *  - parentCategories is the path of current item
   *  - proxyName is the filter proxyname (empty if the current item is a category)
   *  - displayName
   */
  QString populateSettingsValue(const QString& value, QTreeWidgetItem* item);

  QTreeWidgetItem* getSelectedCategory();

  bool eventFilter(QObject* object, QEvent* event) override;

private:
  QScopedPointer<Ui::pqFavoritesDialog> Ui;
};

#endif
