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
  Q_PROPERTY(HeadingLevel headingLevel READ headingLevel WRITE setHeadingLevel)
  Q_ENUMS(HeadingLevel)
public:
  pqExpanderButton(QWidget* parent = nullptr);
  ~pqExpanderButton() override;

  /**
   * This enum specifies the level of the button in a hierarchy of
   * expanders, similar to heading levels in a document. H1 (the
   * default) is for top level sections such as Properties/Display/View
   * (plus/minus icons, border), H2 is for nested groups such as
   * property groups (subtler arrow icons, no border).
   */
  enum HeadingLevel
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
   * This property holds the heading level of the button, which
   * determines its look. By default, the heading level is H1.
   */
  void setHeadingLevel(HeadingLevel level);
  HeadingLevel headingLevel() const { return this->CurrentHeadingLevel; }

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
   * Updates the icon shown based on the current checked state and heading level.
   */
  void updateIcon();

  class pqInternals;
  pqInternals* Internals;
  bool Checked;
  HeadingLevel CurrentHeadingLevel;
};

#endif
