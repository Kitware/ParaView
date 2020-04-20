/*=========================================================================

   Program: ParaView
   Module:  pqPreviewMenuManager.h

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
#ifndef pqPreviewMenuManager_h
#define pqPreviewMenuManager_h

#include "pqApplicationComponentsModule.h"
#include "pqTimer.h" // for pqTimer;
#include <QObject>
#include <QPointer>

class QMenu;
class QAction;

/**
 * @class pqPreviewMenuManager
 * @brief builds and manages menu for preview modes
 *
 * pqPreviewMenuManager populates the menu passed to the constructor with
 * actions to switch the application into preview mode were the user can more
 * faithfully setup annotations and scalar bar before saving out screenshots
 * or animation for a target resolution.
 *
 * The generated menu has a bunch if standard pre-defined items. These can be
 * customized by passing a QStringList to the constructor. Also a item to add
 * custom resolutions is added, so that user can click to add custom
 * resolutions. A set of 5 most recent custom resolutions is preserved
 * across sessions.
 *
 * pqPreviewMenuManager requires that your application uses
 * pqTabbedMultiViewWidget which has been registered with
 * pqApplicationCore singleton as a "MULTIVIEW_WIDGET" manager.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPreviewMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Constructor that fills up default set of resolutions with a predefined set.
   */
  pqPreviewMenuManager(QMenu* parent);

  /**
   * Customize the default actions. The `defaultItems` must contain strings for
   * the form "%d x %d (%s)" e.g. "1024 x 1024 (Label)" or "%d x %d" e.g. "1024
   * x 1024". Any other form will be ignored silently.
   */
  pqPreviewMenuManager(const QStringList& defaultItems, QMenu* parent);

  virtual ~pqPreviewMenuManager();

  /**
   * Provides access to the parent menu.
   */
  QMenu* parentMenu() const;

  /**
   * Enter preview mode for the specified resolution. It \c dx or \c dy is <= 1,
   * then it's same as calling `unlock()`.
   */
  void lockResolution(int dx, int dy);

  /**
   * Exit preview mode.
   */
  void unlock();

private Q_SLOTS:
  void updateEnabledState();
  void addCustom();
  void lockResolution(bool lock);
  void lockResolution(int dx, int dy, QAction* target);
  /**
   * If the resolution is changed through the python shell using the vtkSMViewLayoutProxy
   * the menu needs to respond to it.
   */
  void aboutToShow();

private:
  void updateCustomActions();
  QAction* findAction(int dx, int dy);
  bool prependCustomResolution(int dx, int dy, const QString& label);
  QPointer<QAction> FirstCustomAction;
  pqTimer Timer;

private:
  Q_DISABLE_COPY(pqPreviewMenuManager);
  void init(const QStringList& defaultItems, QMenu* parentMenu);
};

#endif
