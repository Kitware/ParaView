/*=========================================================================

   Program: ParaView
   Module:    pqAnimationTimeToolbar.h

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
#ifndef pqAnimationTimeToolbar_h
#define pqAnimationTimeToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QPointer>
#include <QToolBar>

class pqAnimationTimeWidget;
class pqAnimationScene;

/**
* pqAnimationTimeToolbar is a QToolBar containing a pqAnimationTimeWidget.
* pqAnimationTimeToolbar also ensures that the pqAnimationTimeWidget is
* tracking the animation scene on the active session.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimationTimeToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqAnimationTimeToolbar(const QString& _title, QWidget* _parent = 0)
    : Superclass(_title, _parent)
  {
    this->constructor();
  }
  pqAnimationTimeToolbar(QWidget* _parent = 0)
    : Superclass(_parent)
  {
    this->constructor();
  }

  /**
  * Provides access to the pqAnimationTimeWidget used.
  */
  pqAnimationTimeWidget* animationTimeWidget() const;
private slots:
  void setAnimationScene(pqAnimationScene* scene);

  /**
   * Update the notation and precision for time display.
   */
  void updateTimeDisplay();

private:
  Q_DISABLE_COPY(pqAnimationTimeToolbar)
  void constructor();
  QPointer<pqAnimationTimeWidget> AnimationTimeWidget;
};

#endif
