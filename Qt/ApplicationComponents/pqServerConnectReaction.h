/*=========================================================================

   Program: ParaView
   Module:    pqServerConnectReaction.h

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
#ifndef pqServerConnectReaction_h
#define pqServerConnectReaction_h

#include "pqReaction.h"

class pqServerConfiguration;
class pqServerResource;

/**
* @ingroup Reactions
* Reaction for connecting to a server.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqServerConnectReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqServerConnectReaction(QAction* parent);

  /**
  * Creates a server connection.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  */
  static void connectToServerWithWarning();
  static void connectToServer();

  /**
  * ParaView names server configurations (in pvsc files). To connect to a
  * server using the configuration specified, use this API.
  */
  static bool connectToServerUsingConfigurationName(const char* config_name);

  /**
  * To connect to a server given a configuration, use this API.
  */
  static bool connectToServerUsingConfiguration(const pqServerConfiguration& config);

  /**
  * Connect to server using the resource. This will create a temporary
  * configuration for the resource.
  */
  static bool connectToServer(const pqServerResource& resource);

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override { pqServerConnectReaction::connectToServerWithWarning(); }

private Q_SLOTS:
  /**
  * Updates the state of this reaction.
  */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqServerConnectReaction)
};

#endif
