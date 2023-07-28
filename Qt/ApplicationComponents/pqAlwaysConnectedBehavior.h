// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAlwaysConnectedBehavior_h
#define pqAlwaysConnectedBehavior_h

#include <QObject>

#include "pqApplicationComponentsModule.h"
#include "pqServerResource.h"
#include "pqTimer.h"

/**
 * @ingroup Behaviors
 * pqAlwaysConnectedBehavior ensures that the client always remains connected
 * to a server.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAlwaysConnectedBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAlwaysConnectedBehavior(QObject* parent = nullptr);
  ~pqAlwaysConnectedBehavior() override;

  /**
   * Get/Set the default server resource to connect to.
   */
  void setDefaultServer(const pqServerResource& resource) { this->DefaultServer = resource; }
  const pqServerResource& defaultServer() const { return this->DefaultServer; }

protected Q_SLOTS:
  void serverCheck();

protected: // NOLINT(readability-redundant-access-specifiers)
  pqServerResource DefaultServer;
  pqTimer Timer;

private:
  Q_DISABLE_COPY(pqAlwaysConnectedBehavior)
};

#endif
