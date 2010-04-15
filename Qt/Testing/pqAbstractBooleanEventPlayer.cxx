/*=========================================================================

   Program: ParaView
   Module:    pqAbstractBooleanEventPlayer.cxx

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

#include "pqAbstractBooleanEventPlayer.h"

#include <QAbstractButton>
#include <QGroupBox>
#include <QtDebug>
#include <QAction>

pqAbstractBooleanEventPlayer::pqAbstractBooleanEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqAbstractBooleanEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  if(Command != "set_boolean")
    return false;

  const bool value = Arguments == "true" ? true : false;

  if(QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object))
    {
    if(value != object->isChecked())
      object->click();
    return true;
    }

  if (QAction* const action = qobject_cast<QAction*>(Object))
    {
    if (action->isChecked() != value)
      {
      action->trigger();
      }
    return true;
    }

  if ( QGroupBox* const object =  qobject_cast<QGroupBox*>(Object))
    {
    if ( value != object->isChecked() )
      {
      object->setChecked( value );
      }
    return true;
    }
  qCritical() << "calling set_boolean on unhandled type " << Object;
  Error = true;
  return true;
}

