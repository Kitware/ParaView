/*=========================================================================

   Program: ParaView
   Module:    pqQuickLaunchDialog.h

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
#ifndef pqQuickLaunchDialog_h
#define pqQuickLaunchDialog_h

#include "pqWidgetsModule.h"
#include <QDialog>

/**
* A borderless pop-up dialog used to show actions that the user can launch.
* Provides search capabilities.
*/
class PQWIDGETS_EXPORT pqQuickLaunchDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqQuickLaunchDialog(QWidget* parent = 0);
  ~pqQuickLaunchDialog() override;

  /**
  * Set the actions to be launched using this dialog.
  * This clears all already added actions.
  */
  void setActions(const QList<QAction*>& actions);

  /**
  * Add actions to be launched using this dialog.
  * This adds to already added actions.
  */
  void addActions(const QList<QAction*>& actions);

public Q_SLOTS:
  /**
  * Overridden to trigger the user selected action.
  */
  void accept() override;

protected Q_SLOTS:
  /**
  * Called when the user chooses an item from available choices shown in the
  * options list.
  */
  void currentRowChanged(int);

protected:
  /**
  * Overridden to capture key presses.
  */
  bool eventFilter(QObject* watched, QEvent* event) override;

  /**
  * Given the user entered text, update the GUI.
  */
  void updateSearch();

private:
  Q_DISABLE_COPY(pqQuickLaunchDialog)

  class pqInternal;
  pqInternal* Internal;
};

#endif
