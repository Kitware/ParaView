/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
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
public:
  pqExpanderButton(QWidget* parent = 0);
  ~pqExpanderButton() override;

public Q_SLOTS:
  /**
  * Toggles the state of the checkable button.
  */
  void toggle();

  /**
  * This property holds whether the button is checked. By default, the button
  * is unchecked.
  */
  void setChecked(bool);
  bool checked() const { return this->Checked; }

  /**
  * This property holds the text shown on the button.
  */
  void setText(const QString& text);
  QString text() const;

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

  class pqInternals;
  pqInternals* Internals;
  bool Checked;
};

#endif
