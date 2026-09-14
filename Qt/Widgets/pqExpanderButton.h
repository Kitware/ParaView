// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExpanderButton_h
#define pqExpanderButton_h

#include "pqWidgetsModule.h"
#include <QFrame>

/**
 * pqExpanderButton provides a frame with a toggle mode. This can be used to
 * simulate a toggle button used to expand frames in an accordion style, for
 * example.
 */
class PQWIDGETS_EXPORT pqExpanderButton : public QFrame
{
  Q_OBJECT
  typedef QFrame Superclass;

  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(bool checked READ checked WRITE setChecked)
  Q_PROPERTY(Look look READ look WRITE setLook)
  Q_ENUMS(Look)
public:
  pqExpanderButton(QWidget* parent = nullptr);
  ~pqExpanderButton() override;

  /**
   * This enum specifies the look of the button.
   *
   * H1 (the default) uses plus/minus icons and draws a full
   * border around the button.
   *
   * H2 uses expand/collapse icons and does not draw a
   * border around the button.
   */
  enum Look
  {
    H1 = 0,
    H2
  };

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Toggles the state of the checkable button.
   */
  void toggle();

  /**
   * This property holds whether the button is checked (minus,
   * collapse) which means the content is shown. By default, the
   * button is unchecked (plus, expand) which means the content is not
   * shown.
   */
  void setChecked(bool);
  bool checked() const { return this->Checked; }

  /**
   * This property holds the text shown on the button.
   */
  void setText(const QString& text);
  QString text() const;

  /**
   * This property holds the look of the button. By default, the look
   * is H1.
   */
  void setLook(Look look);
  Look look() const { return this->CurrentLook; }

Q_SIGNALS:
  /**
   * This signal is emitted whenever a button changes its state.
   * checked is true if the button is checked, or false if the button is
   * unchecked.
   */
  void toggled(bool checked);

protected:
  void mousePressEvent(QMouseEvent* evt) override;
  void mouseReleaseEvent(QMouseEvent* evt) override;

private:
  Q_DISABLE_COPY(pqExpanderButton)

  /**
   * Updates the icon shown based on the current checked state and look.
   */
  void updateIcon();

  class pqInternals;
  pqInternals* Internals;
  bool Checked;
  Look CurrentLook;
};

#endif
