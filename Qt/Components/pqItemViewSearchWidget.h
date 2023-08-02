// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqItemViewSearchWidget_h
#define pqItemViewSearchWidget_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QModelIndex>

/**
 * This is the search widget for QAbstractItemView type of widgets.
 * When Ctrl-F is invoked on the view widget, this widget will show
 * up at the top of the view widget, and it will search through the items
 * as user typing in the text field, and highlight the view item
 * containing the input text
 */
class PQCOMPONENTS_EXPORT pqItemViewSearchWidget : public QDialog
{
  Q_OBJECT
  Q_ENUMS(ItemSearchType)
  typedef QDialog Superclass;

public:
  pqItemViewSearchWidget(QWidget* parent = nullptr);
  ~pqItemViewSearchWidget() override;
  enum ItemSearchType
  {
    Current,
    Next,
    Previous
  };

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void showSearchWidget();

protected Q_SLOTS:

  /**
   * Given the user entered text, update the GUI.
   */
  virtual void updateSearch(QString);
  virtual void updateSearch();
  /**
   * Find the next/previous item
   */
  virtual void findNext();
  virtual void findPrevious();

protected: // NOLINT(readability-redundant-access-specifiers)
  virtual void setBaseWidget(QWidget* baseWidget);
  /**
   * Overridden to capture key presses.
   */
  bool eventFilter(QObject* obj, QEvent* event) override;
  void keyPressEvent(QKeyEvent* e) override;
  /**
   * Recursive to search all QModelIndices in the model.
   */
  virtual bool searchModel(const QAbstractItemModel* M, const QModelIndex& Top, const QString& S,
    ItemSearchType searchType = Current) const;
  /**
   * Overwrite to focus the lineEdit box
   */
  void showEvent(QShowEvent*) override;
  /**
   * match the input string with the index's text
   */
  virtual bool matchString(
    const QAbstractItemModel* M, const QModelIndex& curIdx, const QString& searchString) const;

private:
  Q_DISABLE_COPY(pqItemViewSearchWidget)

  class PIMPL;
  PIMPL* Private;
};
#endif
