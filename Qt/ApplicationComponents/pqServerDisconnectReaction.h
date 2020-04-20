/*=========================================================================

   Program: ParaView
   Module:    pqServerDisconnectReaction.h

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
#ifndef pqServerDisconnectReaction_h
#define pqServerDisconnectReaction_h

#include "pqReaction.h"
#include "pqTimer.h" // needed for pqTimer.

/**
* @ingroup Reactions
* Reaction to disconnect from a server.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqServerDisconnectReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqServerDisconnectReaction(QAction* parent);

  /**
  * Disconnects from active server.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  */
  static void disconnectFromServer();

  /**
  * Disconnects from active server with a warning message to the user to
  * confirm that active proxies will be destroyed, if any. Returns true if
  * disconnect happened, false if not i.e. the user cancelled the operation.
  */
  static bool disconnectFromServerWithWarning();
private Q_SLOTS:
  void updateState();

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

  pqTimer UpdateTimer;

private:
  Q_DISABLE_COPY(pqServerDisconnectReaction)
};

#endif
