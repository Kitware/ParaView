/*=========================================================================

   Program: ParaView
   Module:    pqTabBarEventPlayer.cxx

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

#include "pqTabBarEventPlayer.h"

#include <QComboBox>
#include <QLineEdit>
#include <QtDebug>

pqTabBarEventPlayer::pqTabBarEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqTabBarEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  if(Command != "set_tab")
    return false;

  const QString value = Arguments;
    
  if(QTabBar* const object = qobject_cast<QTabBar*>(Object))
    {
    bool ok = false;
    int which = value.toInt(&ok);
    if(!ok)
      {
      qCritical() << "calling set_tab with invalid argument on " << Object;
      Error = true;
      }
    else if(object->count() < which)
      {
      qCritical() << "calling set_tab with out of bounds index on " << Object;
      Error = true;
      }
    else
      {
      object->setCurrentIndex(which);
      }
    return true;
    }

  qCritical() << "calling set_tab on unhandled type " << Object;

  Error = true;
  return true;
}

