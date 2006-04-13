/*=========================================================================

   Program:   ParaQ
   Module:    pqAbstractActivateEventPlayer.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqAbstractActivateEventPlayer.h"
#include "pqTesting.h"

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QPushButton>
#include <QtDebug>

pqAbstractActivateEventPlayer::pqAbstractActivateEventPlayer()
{
}

bool pqAbstractActivateEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& /*Arguments*/, bool& Error)
{
  if(Command != "activate")
    return false;

  if(QAction* const object = qobject_cast<QAction*>(Object))
    {
    if(QMenu* const menu = qobject_cast<QMenu*>(object->parent()))
      {
        if(!menu->parent())
          {
          while(!menu->isVisible())
            pqTesting::NonBlockingSleep(100);
          }

      QKeyEvent key_press(QEvent::KeyPress, Qt::Key_Escape, 0);
      QApplication::sendEvent(menu, &key_press);

      QKeyEvent key_release(QEvent::KeyRelease, Qt::Key_Escape, 0);
      QApplication::sendEvent(menu, &key_release);
      }
      
    object->activate(QAction::Trigger);
    return true;
    }

  if(QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object))
    {
    object->click();
    return true;
    }

  qCritical() << "calling activate on unhandled type " << Object;
  Error = true;
  return true;
}

