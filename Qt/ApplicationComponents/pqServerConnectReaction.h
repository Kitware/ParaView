// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
   * Constructor. Parent cannot be nullptr.
   */
  pqServerConnectReaction(QAction* parent);

  /**
   * Creates a server connection using the server connection dialog.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static void connectToServerWithWarning();
  static void connectToServer();

  /**
   * ParaView names server configurations (in pvsc files). To connect to a
   * server using the configuration specified, use this API.
   */
  static bool connectToServerUsingConfigurationName(
    const char* config_name, bool showConnectionDialog = true);

  /**
   * To connect to a server given a configuration, use this API.
   */
  static bool connectToServerUsingConfiguration(
    const pqServerConfiguration& config, bool showConnectionDialog = true);

  /**
   * Connect to server using the resource. This will create a temporary
   * configuration for the resource.
   */
  static bool connectToServer(const pqServerResource& resource, bool showConnectionDialog = true);

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

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqServerConnectReaction)
};

#endif
