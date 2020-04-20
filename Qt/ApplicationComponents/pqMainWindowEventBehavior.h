/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowEventBehavior.h

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
#ifndef pqMainWindowEventBehavior_h
#define pqMainWindowEventBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QShowEvent;

/**
* @ingroup Reactions
* Reaction to when things are dragged into the main window.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqMainWindowEventBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqMainWindowEventBehavior(QObject* parent = 0);
  virtual ~pqMainWindowEventBehavior();

public Q_SLOTS:
  /**
  * Triggered when a close event occurs on the main window.
  */
  void onClose(QCloseEvent*);

  /**
  * Triggered when a show event occurs on the main window.
  */
  void onShow(QShowEvent*);

  /**
  * Triggered when a drag enter event occurs on the main window.
  */
  void onDragEnter(QDragEnterEvent*);

  /**
  * Triggered when a drop event occurs on the main window.
  */
  void onDrop(QDropEvent*);

private:
  Q_DISABLE_COPY(pqMainWindowEventBehavior)
};

#endif
