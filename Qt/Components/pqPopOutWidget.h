/*=========================================================================

   Program: ParaView
   Module: pqPopOutWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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
#ifndef _pqPopOutWidget_h
#define _pqPopOutWidget_h

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
  pqPopOutWidget(QWidget* widgetToPopOut, const QString& dialogTitle, QWidget* p = 0);
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

#endif // PQPOPOUTWIDGET_H
