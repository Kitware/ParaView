// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPopOutWidget_h
#define pqPopOutWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class QLayout;
class QPushButton;

/**
 * This pqPopOutWidget provides a mechanism to pop out its contained
 * widget into a dialog return it to its prior location.
 *
 * As layout and contained widgets are managed internally, do not call
 * its inherited functions that modify these.
 */

class PQCOMPONENTS_EXPORT pqPopOutWidget : public QWidget
{
  Q_OBJECT
public:
  /**
   * Constructs a pqPopOutWidget wrapping the given widget and using
   * the given string as the title of the dialog when the widget is
   * popped out.
   */
  pqPopOutWidget(QWidget* widgetToPopOut, const QString& dialogTitle, QWidget* p = nullptr);
  ~pqPopOutWidget() override;

  /**
   * Sets the button that will control when the widget is popped out
   * to the dialog.  This function connects the button to the
   * appropriate slots and will cause its icon to be updated based on
   * whether the dialog is visible or not.
   */
  void setPopOutButton(QPushButton* button);

Q_SIGNALS:

protected Q_SLOTS:
  /**
   * Moves the widget to the other location.
   */
  void toggleWidgetLocation();
  /**
   * Moves the widget to the dialog and shows the dialog if the widget
   * is not in the dialog.  If the widget is already in the dialog, this
   * function does nothing.
   */
  void moveWidgetToDialog();
  /**
   * Moves the widget from the dialog back to being a child of this widget.
   * This slot also hides the dialog if it is visible.  This slot does
   * nothing if the widget is not in the dialog and is automatically called
   * when the dialog is closed.
   */
  void moveWidgetBackToParent();

private:
  class pqInternal;
  pqInternal* Internals;
};

#endif // pqPopOutWidget_h
