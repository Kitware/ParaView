// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqQuickLaunchDialogExtended_h
#define pqQuickLaunchDialogExtended_h

#include "pqApplicationComponentsModule.h"

#include <QDialog>
#include <QList> // for QList

#include <memory> // for unique_ptr

class pqProxyActionListModel;
class pqExtendedSortFilterProxyModel;
class QListView;
class QSortFilterProxyModel;

namespace Ui
{
class QuickLaunchDialogExtended;
}

/**
 * \brief: A pop-up dialog used to browse and create proxies.
 *
 * \details: pqQuickLaunchDialogExtended is a Dialog intended for quick
 * navigation through proxies like filters and sources.
 *
 * The user text input is used to search in proxies name and documentation.
 * Two lists are displayed: the top one contains proxies that are available
 * The second contains the disabled ones.
 *
 * Help and missing requirements are displayed for the selected proxy.
 *
 * This dialog is keyboard-oriented:
 *  - type text to search
 *  - arrows and tabs to navigate in the lists
 *  - enter to create proxy
 *  - Esc to quit
 *
 * Note that mouse interaction is still possible to select and create proxies.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqQuickLaunchDialogExtended : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqQuickLaunchDialogExtended(QWidget* parent, const QList<QAction*>& actions);
  ~pqQuickLaunchDialogExtended() override;

Q_SIGNALS:
  /**
   * Emited when a pipeline update is request by the user.
   * Typically emited from a Shift+Enter event.
   */
  void applyRequested();

protected Q_SLOTS:
  /**
   * Updates the dialog with the given user request.
   * This filters the proxies lists.
   * Force focus to the first available proxy if any. Fallback to the first disabled proxy
   * otherwise.
   */
  void requestChanged(const QString& request);

  /**
   * Updates the dialog when the current selected item changed.
   * This mainly display its documentation and requirements when relevant.
   */
  void currentChanged(const QModelIndex& currentIndex, QSortFilterProxyModel* proxyModel);

  ///@{
  /**
   * Creates the current proxy in the pipeline.
   * That will also close the window.
   */
  void createCurrentProxy(bool autoApply);
  void createCurrentProxyWithoutApply();
  ///@}

  /**
   * Handles text changed event.
   */
  void handleTextChanged(int key);

  /**
   * Move in proxy list
   */
  void move(int key);

  /**
   * Update focus between both list views.
   */
  void toggleFocus();

  /**
   * Clear request or quit if request is already empty
   */
  void cancel();

  /**
   * Open the Help panel for current proxy. This will also close the dialog.
   */
  void showProxyHelp();

private:
  /**
   * Set the current item in the given list.
   * Clear the selection in the other list.
   */
  void makeCurrent(QListView* view, int row);

  /**
   * Return the current list, i.e. the one with a selected element.
   */
  QListView* currentList();

  std::unique_ptr<pqProxyActionListModel> Model;
  std::unique_ptr<pqExtendedSortFilterProxyModel> AvailableSortFilterModel;
  std::unique_ptr<pqExtendedSortFilterProxyModel> DisabledSortFilterModel;

  std::unique_ptr<Ui::QuickLaunchDialogExtended> Ui;
};

#endif
