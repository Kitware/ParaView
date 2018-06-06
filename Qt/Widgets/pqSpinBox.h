/*=========================================================================

   Program: ParaView
   Module:  pqSpinBox.h

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

========================================================================*/
#ifndef pqSpinBox_h
#define pqSpinBox_h

#include "pqWidgetsModule.h"
#include <QSpinBox>

/**
* QSpinBox which fires editingFinished() signal when the value is changed
* by steps (increments).
* Also, this adds a new signal valueChangedAndEditingFinished() which is fired
* after editingFinished() signal is fired and the value in the spin box indeed
* changed.
*/
class PQWIDGETS_EXPORT pqSpinBox : public QSpinBox
{
  Q_OBJECT
  typedef QSpinBox Superclass;

public:
  explicit pqSpinBox(QWidget* parent = 0);

  /**
  * Virtual function that is called whenever the user triggers a step.  We are
  * overriding this so that we can emit editingFinished() signal
  */
  void stepBy(int steps) override;

signals:
  /**
  * Unlike QSpinBox::editingFinished() which gets fired whenever the widget
  * looses focus irrespective of if the value was indeed edited,
  * valueChangedAndEditingFinished() is fired only when the value was changed
  * as well.
  */
  void valueChangedAndEditingFinished();

private slots:
  void onValueEdited();
  void onEditingFinished();

private:
  Q_DISABLE_COPY(pqSpinBox)
  bool EditingFinishedPending;
};

#endif
