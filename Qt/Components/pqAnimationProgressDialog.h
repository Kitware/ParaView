/*=========================================================================

   Program: ParaView
   Module:  pqAnimationProgressDialog.h

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
#ifndef pqAnimationProgressDialog_h
#define pqAnimationProgressDialog_h

#include "pqComponentsModule.h" // for exports macro
#include <QProgressDialog>

class pqAnimationScene;
class vtkSMProxy;

/**
 * @class pqAnimationProgressDialog
 * @brief progress dialog for animation progress
 *
 * pqAnimationProgressDialog is a QProgressDialog that hooks itself up to
 * monitor (and show) progress from an animation scene. The dialog uses the
 * cancel button to abort the animation playback.
 *
 * Typical usage:
 *
 * @code{.cpp}
 *   pqAnimationProgressDialog progress(
 *   "Save animation progress", "Abort", 0, 100, pqCoreUtilities::mainWidget());
 *   progress.setWindowTitle("Saving Animation ...");
 *   progress.setAnimationScene(scene);
 *   progress.show();
 *
 *   auto appcore = pqApplicationCore::instance();
 *   auto pgm = appcore->getProgressManager();
 *   // this is essential since pqProgressManager blocks all interaction
 *   // events when progress events are pending. since we have a QProgressDialog
 *   // as modal, we don't need to that. Plus, we want the cancel button on the
 *   // dialog to work.
 *   const auto prev = pgm->unblockEvents(true);
 *
 *       // ---- do animation playback ---
 *
 *   pgm->unblockEvents(prev);
 *   progress.hide();
 * @endcode
 */
class PQCOMPONENTS_EXPORT pqAnimationProgressDialog : public QProgressDialog
{
  Q_OBJECT
  typedef QProgressDialog Superclass;

public:
  pqAnimationProgressDialog(const QString& labelText, const QString& cancelButtonText,
    int minimum = 0, int maximum = 100, QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags());
  pqAnimationProgressDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqAnimationProgressDialog() override;

  //@{
  /**
   * Set the animation scene to monitor.
   */
  void setAnimationScene(pqAnimationScene*);
  void setAnimationScene(vtkSMProxy*);
  //@}

private:
  Q_DISABLE_COPY(pqAnimationProgressDialog);
  QMetaObject::Connection Connection;
};

#endif
