/*=========================================================================

   Program: ParaView
   Module:  pqExtractorsMenuReaction.h

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
#ifndef pqExtractorsMenuReaction_h
#define pqExtractorsMenuReaction_h

#include "pqApplicationComponentsModule.h" // for exports
#include "pqTimer.h"                       // for pqTimer
#include <QObject>

class pqExtractor;
class pqProxyGroupMenuManager;

class PQAPPLICATIONCOMPONENTS_EXPORT pqExtractorsMenuReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqExtractorsMenuReaction(pqProxyGroupMenuManager* menuManager, bool hideDisabledActions = false);
  ~pqExtractorsMenuReaction() override;

public Q_SLOTS:
  /**
   * Updates the enabled state for actions in the menu assigned to the
   * menuManager.
   */
  void updateEnableState(bool updateOnlyToolbars = false);

  /**
   * Creates an extract generator.
   */
  pqExtractor* createExtractor(const QString& group, const QString& name) const;

private:
  Q_DISABLE_COPY(pqExtractorsMenuReaction);
  bool HideDisabledActions;
  pqTimer Timer;
};

#endif
