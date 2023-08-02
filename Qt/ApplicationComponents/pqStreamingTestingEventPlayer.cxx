// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqStreamingTestingEventPlayer.h"
#include "pqViewStreamingBehavior.h"

//-----------------------------------------------------------------------------
bool pqStreamingTestingEventPlayer::playEvent(
  QObject*, const QString& command, const QString& arguments, bool& error)
{
  if (command == "pqViewStreamingBehavior" && this->StreamingBehavior)
  {
    if (arguments == "stop_streaming")
    {
      this->StreamingBehavior->stopAutoUpdates();
    }
    else if (arguments == "resume_streaming")
    {
      this->StreamingBehavior->resumeAutoUpdates();
    }
    else if (arguments == "next")
    {
      this->StreamingBehavior->triggerSingleUpdate();
    }
    else
    {
      error = true;
    }
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
void pqStreamingTestingEventPlayer::setViewStreamingBehavior(pqViewStreamingBehavior* vsBehavior)
{
  this->StreamingBehavior = vsBehavior;
}

//-----------------------------------------------------------------------------
pqViewStreamingBehavior* pqStreamingTestingEventPlayer::viewStreamingBehavior()
{
  return this->StreamingBehavior;
}
