/*=========================================================================

   Program: ParaView
   Module:  pqDoubleLineEditEventPlayer.cxx

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

// Qt Includes.
#include <QDebug>

// ParaView Includes.
#include "pqDoubleLineEdit.h"
#include "pqDoubleLineEditEventPlayer.h"

//-----------------------------------------------------------------------------
pqDoubleLineEditEventPlayer::pqDoubleLineEditEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqDoubleLineEditEventPlayer::~pqDoubleLineEditEventPlayer()
{
}

//-----------------------------------------------------------------------------
bool pqDoubleLineEditEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& error)
{
  pqDoubleLineEdit* doubleLineEdit = qobject_cast<pqDoubleLineEdit*>(object);
  if (!doubleLineEdit)
  {
    return false;
  }
  if (command == Self::EVENT_NAME() || command == "set_string")
  {
    doubleLineEdit->setFullPrecisionText(arguments);
    doubleLineEdit->triggerFullPrecisionTextChangedAndEditingFinished();
    return true;
  }
  return this->Superclass::playEvent(object, command, arguments, error);
}

//-----------------------------------------------------------------------------
const QString& pqDoubleLineEditEventPlayer::EVENT_NAME()
{
  static const QString eventName("set_full_precision_text");
  return eventName;
}
