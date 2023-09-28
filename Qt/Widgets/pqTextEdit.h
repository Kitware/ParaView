// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTextEdit_h
#define pqTextEdit_h

#include "pqWidgetsModule.h"

#include <QTextEdit>

class pqTextEditPrivate;
class pqWidgetCompleter;

/**
 * pqTextEdit is a specialization of QTextEdit which provide
 * editingFinished() and textChangedAndEditingFinished() signals,
 * as well as the possibility to be autocompleted.
 *
 * An autocompleter object can be set using setCompleter(). For this, autocompleter must
 * implement the updateCompletionModel() method through the interface defined
 * by pqWidgetCompleter, providing completion for a given string.
 *
 * Unlike editingFinished() which gets fired whenever the widget looses
 * focus irrespective of if the text if actually was edited,
 * textChangedAndEditingFinished() is fired only when the text was changed
 * as well.
 *
 * Important Notes:
 * The editingFinished() signals and the textChangedAndEditingFinished()
 * are *NOT* sent when using the setText, setPlainText and setHtml methods.
 *
 * Also the textChangedAndEditingFinished() is not truly emitted only when
 * the text has changed and the edition is finished. For example, removing
 * a letter and adding it back will cause the signal to be fired even though
 * the text is the same as before.
 */
class PQWIDGETS_EXPORT pqTextEdit : public QTextEdit
{
  Q_OBJECT
  typedef QTextEdit Superclass;

public:
  pqTextEdit(QWidget* parent = nullptr);
  pqTextEdit(const QString& contents, QWidget* parent = nullptr);

  void setCompleter(pqWidgetCompleter* completer);
  pqWidgetCompleter* getCompleter() { return this->Completer; }

  ~pqTextEdit() override;

Q_SIGNALS:
  /**
   * Unlike editingFinished() which gets fired whenever the widget looses
   * focus irrespective of if the text actually was edited,
   * textChangedAndEditingFinished() is fired only when the text was changed
   * as well.
   */
  void textChangedAndEditingFinished();

  /**
   * Just like the QLineEdit::editingFinished(), this signal is fired every
   * time the widget loses focus.
   */
  void editingFinished();

private Q_SLOTS:
  void onEditingFinished();
  void onTextEdited();
  void insertCompletion(const QString& completion);

protected:
  void keyPressEvent(QKeyEvent* e) override;
  void focusOutEvent(QFocusEvent* e) override;
  void focusInEvent(QFocusEvent* e) override;

  /**
   * Returns the text of the current line in the input field.
   */
  QString textUnderCursor() const;

  /**
   * Trigger an update on the completer if any, and show the popup if completions are available.
   */
  void updateCompleter();

  /**
   * In case the completer popup menu is already shown, update it to reflect modifications on the
   * input text.
   */
  void updateCompleterIfVisible();

  /**
   * Insert the completion in case there is only one available.
   */
  void selectCompletion();

  QScopedPointer<pqTextEditPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(pqTextEdit);
  Q_DISABLE_COPY(pqTextEdit);

  pqWidgetCompleter* Completer = nullptr;
};

#endif
