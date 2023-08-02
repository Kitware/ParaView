// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLineEdit_h
#define pqLineEdit_h

#include "pqWidgetsModule.h"
#include <QLineEdit>
/**
 * pqLineEdit is a specialization of QLineEdit which provide a new property
 * 'text2'. When the text on the line widget is set using this 'text2' property
 * (or using setTextAndResetCursor()), the after the set, the cursor position
 * is reset to 0.
 *
 * Additional, this provides a true editingFinished() signal under the name of
 * textChangedAndEditingFinished(). Unlike QLineEdit::editingFinished() which
 * gets fired whenever the widget looses focus irrespective of if the text
 * actually was edited, textChangedAndEditingFinished() is fired only when the
 * text was changed as well.
 *
 * To enable/disable whether the cursor position is reset to 0 after
 * textChangedAndEditingFinished() if fired, use the
 * resetCursorPositionOnEditingFinished property (default: true).
 */
class PQWIDGETS_EXPORT pqLineEdit : public QLineEdit
{
  Q_OBJECT
  Q_PROPERTY(QString text2 READ text WRITE setTextAndResetCursor)
  Q_PROPERTY(bool resetCursorPositionOnEditingFinished READ resetCursorPositionOnEditingFinished
      WRITE setResetCursorPositionOnEditingFinished)

  typedef QLineEdit Superclass;

public:
  pqLineEdit(QWidget* parent = nullptr);
  pqLineEdit(const QString& contents, QWidget* parent = nullptr);

  ~pqLineEdit() override;

  /**
   * To enable/disable whether the cursor position is reset to 0 after
   * editingFinished() is fired, use the resetCursorPositionOnEditingFinished
   * property (default: true).
   */
  bool resetCursorPositionOnEditingFinished() const
  {
    return this->ResetCursorPositionOnEditingFinished;
  }

Q_SIGNALS:
  /**
   * Unlike QLineEdit::editingFinished() which
   * gets fired whenever the widget looses focus irrespective of if the text
   * actually was edited, textChangedAndEditingFinished() is fired only when the
   * text was changed as well.
   */
  void textChangedAndEditingFinished();

  /**
   * This signal is never fired.
   * It exists so that you can add a unidirectional property link, where an
   * information-only property updates the pqLineEdit contents, but user edits
   * to the pqLineEdit never update the property.
   *
   * \sa pqCoordinateFramePropertyWidget which uses this so users can enter
   * coordinates for an axis without the property being updated (which would
   * cause other values to be modified, interrupting the user's edit).
   */
  void blank();

public Q_SLOTS:
  /**
   * Same as QLineEdit::setText() except that it reset the cursor position to
   * 0.  This is useful with the pqLineEdit is used for showing numbers were
   * the digits on the left are more significant on the right.
   */
  void setTextAndResetCursor(const QString& text);

  /**
   * To enable/disable whether the cursor position is reset to 0 after
   * editingFinished() is fired, use the
   * resetCursorPositionOnEditingFinished property (default: true).
   */
  void setResetCursorPositionOnEditingFinished(bool val)
  {
    this->ResetCursorPositionOnEditingFinished = val;
  }

private Q_SLOTS:
  void onTextEdited();
  void onEditingFinished();

protected:
  friend class pqLineEditEventPlayer;
  /**
   * this is called by pqLineEditEventPlayer during event playback to ensure
   * that the textChangedAndEditingFinished() signal is fired when text is changed
   * using setText() in playback.
   */
  void triggerTextChangedAndEditingFinished();

  // Override to select all text in the widget when it gains focus
  void focusInEvent(QFocusEvent* event) override;

private:
  Q_DISABLE_COPY(pqLineEdit)

  bool EditingFinishedPending;
  bool ResetCursorPositionOnEditingFinished;
};

#endif
