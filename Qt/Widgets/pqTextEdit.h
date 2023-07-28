// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTextEdit_h
#define pqTextEdit_h

#include "pqWidgetsModule.h"
#include <QTextEdit>

class pqTextEditPrivate;

/**
 * pqTextEdit is a specialization of QTextEdit which provide a
 * editingFinished() signal and a textChangedAndEditingFinished().
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

protected:
  void keyPressEvent(QKeyEvent* e) override;
  void focusOutEvent(QFocusEvent* e) override;

  QScopedPointer<pqTextEditPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(pqTextEdit);
  Q_DISABLE_COPY(pqTextEdit)
};

#endif
