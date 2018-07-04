/*=========================================================================

   Program: ParaView
   Module:    pqReaction.h

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
#ifndef pqReaction_h
#define pqReaction_h

#include "pqApplicationComponentsModule.h"
#include <QAction>
#include <QObject>

/**
* @defgroup Reactions ParaView Reactions
* ParaView client relies on a collection of reactions that are autonomous
* entities that use pqApplicationCore and other core components to get their
* work done. To use, simply attach an instance of pqReaction subclass to a
* QAction. The reaction then monitors events from the QAction. Additionally, the
* reaction can monitor the ParaView application state to update things like
* enable state, label, etc. for the QAction itself.
*/

/**
* @ingroup Reactions
* This is a superclass just to make it easier to collect all such reactions.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqReaction(QAction* parent, Qt::ConnectionType type = Qt::AutoConnection);
  ~pqReaction() override;

  /**
  * Provides access to the parent action.
  */
  QAction* parentAction() const { return qobject_cast<QAction*>(this->parent()); }

protected slots:
  /**
  * Called when the action is triggered.
  */
  virtual void onTriggered() {}

  virtual void updateEnableState() {}
  virtual void updateMasterEnableState(bool);

protected:
  bool IsMaster;

private:
  Q_DISABLE_COPY(pqReaction)
};

#endif
