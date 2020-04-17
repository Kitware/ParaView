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
#ifndef pqObjectPickingBehavior_h
#define pqObjectPickingBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPoint>

class pqView;
class pqOutputPort;

/**
* @ingroup Behaviors
* pqObjectPickingBehavior is used to add support for picking "source" by
* clicking on it in a view. This currently only supports render-view. But we
* can add support for other views as needed.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqObjectPickingBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqObjectPickingBehavior(QObject* parent = 0);
  ~pqObjectPickingBehavior() override;

protected:
  /**
  * event filter to capture the left-click.
  */
  bool eventFilter(QObject* caller, QEvent* e) override;

protected Q_SLOTS:
  /**
  * Called when a new view is added. We install event-filter to get click
  * events.
  */
  void onViewAdded(pqView*);

private:
  Q_DISABLE_COPY(pqObjectPickingBehavior)
  QPoint Position;
};

#endif
