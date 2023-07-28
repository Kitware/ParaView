// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCollaborationBehavior_h
#define pqCollaborationBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqServer;
class pqCollaborationManager;

/**
 * @ingroup Behaviors
 * pqCollaborationBehavior ensures that a pqCollaborationManager get set
 * when a new pqServer that support collaboration has been created.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCollaborationBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCollaborationBehavior(QObject* parent = nullptr);

private:
  Q_DISABLE_COPY(pqCollaborationBehavior)

  pqCollaborationManager* CollaborationManager;
};

#endif
