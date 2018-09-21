/*=========================================================================

   Program: ParaView
   Module:    pqFavoritesDialog.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqFavoritesDialog_h
#define _pqFavoritesDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QPointer>

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
class PQCOMPONENTS_EXPORT pqFavoritesDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqFavoritesDialog(const QVariant& filtersList, QWidget* p = nullptr);
  ~pqFavoritesDialog() override;

protected slots:
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

protected:
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
