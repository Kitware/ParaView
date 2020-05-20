/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateAndScreenshotReaction.h

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
#ifndef pqSaveStateAndScreenshotReaction_h
#define pqSaveStateAndScreenshotReaction_h

#include "pqReaction.h"

#include "vtkSmartPointer.h"
#include <QPointer>
#include <QString>

class vtkSMProxy;
class vtkSMSaveScreenshotProxy;
class pqView;

/**
 * @ingroup Reactions
 * pqSaveStateAndScreenshotReaction is a reaction to Save State and Screenshot
 */
class pqSaveStateAndScreenshotReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqSaveStateAndScreenshotReaction(QAction* saveAction, QAction* settingsAction);
  ~pqSaveStateAndScreenshotReaction() override = default;

public slots:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected slots:
  /**
   * Called when the Save action is triggered
   */
  void onTriggered() override;
  /**
   * Called on the active view changed to check if the layout changed.
   * If that happens, we deactivate the save button to force the user to save settings
   */
  void onViewChanged(pqView*);

  /**
   * Called when the Settings action is triggered
   */
  void onSettings();

private:
  void CopyProperties(vtkSMSaveScreenshotProxy* shProxySaved, vtkSMSaveScreenshotProxy* shProxy);

private:
  Q_DISABLE_COPY(pqSaveStateAndScreenshotReaction);

  QString Directory;
  QString Name;
  bool FromCTest;
  // vtkSaveScreenshotProxy
  vtkSmartPointer<vtkSMProxy> Proxy;
  QPointer<QAction> SettingsAction;
};

#endif
