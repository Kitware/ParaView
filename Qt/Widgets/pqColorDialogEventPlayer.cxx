/*=========================================================================

   Program: ParaView
   Module:  pqColorDialogEventPlayer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqColorDialogEventPlayer.h"

#include <QColorDialog>
#include <QRegExp>

//----------------------------------------------------------------------------
pqColorDialogEventPlayer::pqColorDialogEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//----------------------------------------------------------------------------
pqColorDialogEventPlayer::~pqColorDialogEventPlayer() = default;

//-----------------------------------------------------------------------------
bool pqColorDialogEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& /*error*/)
{
  QColorDialog* dialog = qobject_cast<QColorDialog*>(object);
  if (!dialog)
  {
    return false;
  }

  QRegExp regExp("^(\\d+),(\\d+),(\\d+)$");
  if ((command == pqColorDialogEventPlayer::EVENT_NAME()) && (regExp.indexIn(arguments) != -1))
  {
    QColor rgb(regExp.cap(1).toInt(), regExp.cap(2).toInt(), regExp.cap(3).toInt());
    dialog->setCurrentColor(rgb);
    return true;
  }
  else if (command == "done")
  {
    static_cast<QDialog*>(dialog)->done(arguments.toInt());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const QString& pqColorDialogEventPlayer::EVENT_NAME()
{
  static const QString eventName("setChosenColor");
  return eventName;
}
