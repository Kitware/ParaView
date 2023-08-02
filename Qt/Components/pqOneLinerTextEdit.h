// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOneLinerTextEdit_h
#define pqOneLinerTextEdit_h

#include "pqComponentsModule.h"
#include "pqTextEdit.h"

/**
 * pqOneLinerTextEdit is a specialization of pqTextEdit to handle one-liner expressions.
 *
 * This widget differs from pqTextEdit with the following:
 *  * New lines characters are disabled from input event and cleaned from pasted text.
 *  * Text is wrapped to fit the widget width.
 *  * Widget height is adapted to the content, so the whole text is displayed.
 */
class PQCOMPONENTS_EXPORT pqOneLinerTextEdit : public pqTextEdit
{
  Q_OBJECT
  typedef pqTextEdit Superclass;

public:
  pqOneLinerTextEdit(QWidget* parent = nullptr);
  ~pqOneLinerTextEdit() override = default;

protected:
  /**
   * Reimplemented to disable "Enter" and "Return" key
   */
  void keyPressEvent(QKeyEvent* e) override;

  /**
   * Reimplemented to update widget height depending on
   * text wrapping.
   */
  void resizeEvent(QResizeEvent* e) override;

  /**
   * Reimplemented to avoid carriage return in pasted text.
   * Also remove redundant spaces.
   * see QString::simplified()
   */
  void insertFromMimeData(const QMimeData* source) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Adjust widget height to the number of displayed lines.
   */
  void adjustToText();

private:
  Q_DISABLE_COPY(pqOneLinerTextEdit)
};

#endif
