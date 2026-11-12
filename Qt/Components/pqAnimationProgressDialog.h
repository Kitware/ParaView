// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
 *   pgm->addNonBlockableObject(progress);
 *   pgm->setEnableAbort(true);
 *   // ---- do animation playback ---
 *   pgm->setEnableAbort(false);
 *   pgm->removeNonBlockableObject(progress);
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

  ///@{
  /**
   * Set the animation scene to monitor.
   */
  void setAnimationScene(pqAnimationScene*);
  void setAnimationScene(vtkSMProxy*);
  ///@}

private:
  Q_DISABLE_COPY(pqAnimationProgressDialog);
  QMetaObject::Connection Connection;
};

#endif
