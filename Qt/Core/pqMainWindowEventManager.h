/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowEventManager.h

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

=========================================================================*/

#ifndef _pqMainWindowEventManager_h
#define _pqMainWindowEventManager_h

#include "pqCoreModule.h"
#include <QObject>

class QCloseEvent;
class QDragEnterEvent;
class QShowEvent;
class QDropEvent;

/**
* pqMainWindowEventManager is a manager for marshalling a main window's events
* to pqReactions.
*
* ParaView and ParaView-derived applications have their own main window class
* that inherits from QMainWindow. The standard Qt approach to intercepting
* signals to the main window (e.g. "close application") is to reimplement
* QMainWindow's event methods (e.g. closeEvent(QCloseEvent*)). This polymorphic
* solution is not available to ParaView-derived applications comprised of
* plugins, however. To facilitate a plugin's ability to influence the behavior
* of ParaView's main window, pqMainWindowEventManager is a registered manager
* that emits signals when the main window receives events.
*/
class PQCORE_EXPORT pqMainWindowEventManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqMainWindowEventManager(QObject* p = 0);
  ~pqMainWindowEventManager() override;

  /**
  * notify when a close event has been sent to the main window.
  */
  void closeEvent(QCloseEvent*);

  /**
  * notify when a show event has been sent to the main window.
  */
  void showEvent(QShowEvent*);

  /**
  * notify when a drag enter event has been sent to the main window.
  */
  void dragEnterEvent(QDragEnterEvent*);

  /**
  * notify when a drop event has been sent to the main window.
  */
  void dropEvent(QDropEvent*);

signals:
  /**
  * notification when a close event has been sent to the main window.
  */
  void close(QCloseEvent*);

  /**
  * notification when a show event has been sent to the main window.
  */
  void show(QShowEvent*);

  /**
  * notification when a drag enter event has been sent to the main window.
  */
  void dragEnter(QDragEnterEvent*);

  /**
  * notification when a drop event has been sent to the main window.
  */
  void drop(QDropEvent*);
};

#endif
