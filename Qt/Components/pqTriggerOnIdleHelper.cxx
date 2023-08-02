// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTriggerOnIdleHelper.h"

#include "pqServer.h"

//-----------------------------------------------------------------------------
pqTriggerOnIdleHelper::pqTriggerOnIdleHelper(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Timer.setInterval(0);
  this->Timer.setSingleShot(true);
  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(triggerInternal()));
}

//-----------------------------------------------------------------------------
pqTriggerOnIdleHelper::~pqTriggerOnIdleHelper() = default;

//-----------------------------------------------------------------------------
pqServer* pqTriggerOnIdleHelper::server() const
{
  return this->Server;
}

//-----------------------------------------------------------------------------
void pqTriggerOnIdleHelper::setServer(pqServer* s)
{
  this->Server = s;
}

//-----------------------------------------------------------------------------
void pqTriggerOnIdleHelper::trigger()
{
  this->Timer.start();
}

//-----------------------------------------------------------------------------
void pqTriggerOnIdleHelper::triggerInternal()
{
  if (this->Server && this->Server->isProgressPending())
  {
    this->trigger();
  }
  else
  {
    Q_EMIT this->triggered();
  }
}
