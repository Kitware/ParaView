// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqExpressionsDialog
#define pqExpressionsDialog

#include "pqComponentsModule.h"
#include <QDialog>

#include <memory> // for unique_ptr

/**
 * pqExpressionsManagerDialog is a dialog class to edit and export expressions.
 *
 * Expressions are loaded from the settings, and are saved on close.
 * Expressions can be exported to a *.json file.
 * When importing a *.json file, new expressions are appended to the current list.
 * Already existing expressions are not duplicated.
 *
 * Expected formatting of the file:
 * \verbatim
 * {
 *   "version": "<version>",
 *    "Expressions": [
 *      {
 *        "Expression": <expressions string>,
 *        "Name": "<expression name>",
 *        "Group": "<expression_group>"
 *      }
 *    ],
 * }
 * \endverbatim
 */
class PQCOMPONENTS_EXPORT pqExpressionsManagerDialog : public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT

public:
  pqExpressionsManagerDialog(QWidget* parent, const QString& group = "");
  ~pqExpressionsManagerDialog() override;

Q_SIGNALS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Emitted after click on "UseCurrent" button. Argument is the first selected expression.
   */
  void expressionSelected(const QString& expr);

protected Q_SLOTS:
  /**
   * Add a new empty expression in the current group, and select it.
   * If there is already such empty expression, use it instead of adding a new one.
   */
  void addNewExpression();

  /**
   * Remove the selected expressions.
   */
  void removeSelectedExpressions();
  /**
   * Remove all expressions.
   */
  void removeAllExpressions();

  /**
   * Save window state and store inner model to settings.
   */
  void onClose();

  /**
   * Update the dialog according to the current state.
   * Enable / disables buttons based on selection.
   */
  void updateUi();

  /**
   * Update table filtering based on group.
   */
  void filterGroup();

  ///@{
  /**
   * Export / import expressions to (respectively from) file.
   * Open a dialog to get the *.json file.
   */
  void exportToFile();
  bool importFromFile();
  ///@}

  /**
   * Triggered by "UseCurrent" button.
   * Emit signal expressionSelected().
   */
  void onUseCurrent();

  /**
   * Reimplemented to handle del
   */
  void keyReleaseEvent(QKeyEvent* event) override;

  /**
   * Save expressions to settings.
   */
  void save();

private:
  Q_DISABLE_COPY(pqExpressionsManagerDialog)

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
