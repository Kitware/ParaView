/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsDialog.h

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
