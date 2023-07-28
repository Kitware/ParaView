// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLineEditEventPlayer.h"

#include "pqLineEdit.h"

//-----------------------------------------------------------------------------
pqLineEditEventPlayer::pqLineEditEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqLineEditEventPlayer::~pqLineEditEventPlayer() = default;

//-----------------------------------------------------------------------------
bool pqLineEditEventPlayer::playEvent(
  QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  bool retval = this->Superclass::playEvent(Object, Command, Arguments, Error);
  if (retval && !Error)
  {
    if (pqLineEdit* object = qobject_cast<pqLineEdit*>(Object))
    {
      object->triggerTextChangedAndEditingFinished();
    }
  }

  return retval;
}
