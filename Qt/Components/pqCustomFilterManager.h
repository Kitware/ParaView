// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \file pqCustomFilterManager.h
 * \date 6/23/2006
 */

#ifndef pqCustomFilterManager_h
#define pqCustomFilterManager_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QStringList>

class pqCustomFilterManagerForm;
class pqCustomFilterManagerModel;
class QItemSelection;

/**
 * \class pqCustomFilterManager
 * \brief
 *   The pqCustomFilterManager class displays the list of registered
 *   custom filter definitions.
 *
 * The custom filter manager uses a pqCustomFilterManagerModel to get
 * the list of registered custom filters. The custom filter manager
 * uses the server manager to import and export custom filter
 * definitions. It can also unregister the selected custom filter.
 */
class PQCOMPONENTS_EXPORT pqCustomFilterManager : public QDialog
{
  Q_OBJECT

public:
  /**
   * \brief
   *   Creates a custom filter manager dialog.
   * \param model The list of registered custom filters to display.
   * \param parent The parent widget for the dialog.
   */
  pqCustomFilterManager(pqCustomFilterManagerModel* model, QWidget* parent = nullptr);
  ~pqCustomFilterManager() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * \brief
   *   Selects the given custom filter in the list.
   * \param name The custom filter name to select.
   */
  void selectCustomFilter(const QString& name);

  /**
   * \brief
   *   Registers the custom filter definitions in the files.
   * \param files The list of files to import.
   */
  void importFiles(const QStringList& files);

  /**
   * \brief
   *   Saves the selected custom filter definitions to the given files.
   * \param files The list of files to export to.
   */
  void exportSelected(const QStringList& files);

private Q_SLOTS:
  /**
   * \brief
   *   Opens the file dialog to select import files.
   * \sa pqCustomFilterManager::importFiles(const QStringList &)
   */
  void importFiles();

  /**
   * \brief
   *   Opens the file dialog to select export files.
   * \sa pqCustomFilterManager::exportSelected(const QStringList &)
   */
  void exportSelected();

  /**
   * Unregisters the selected custom filter definitions.
   */
  void removeSelected();

  /**
   * \brief
   *   Updates the dialog buttons based on the selection.
   *
   * If there is no selection, the export and remove buttons are
   * disabled.
   *
   * \param selected The list of newly selected items.
   * \param deselected The list of deselected items.
   */
  void updateButtons(const QItemSelection& selected, const QItemSelection& deselected);

protected:
  QString getUnusedFilterName(const QString& group, const QString& name);

private:
  pqCustomFilterManagerModel* Model; ///< Stores the custom filter list.
  pqCustomFilterManagerForm* Form;   ///< Defines the gui layout.
};

#endif
