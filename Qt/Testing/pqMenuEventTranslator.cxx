/*=========================================================================

   Program: ParaView
   Module:    pqMenuEventTranslator.cxx

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

#include "pqMenuEventTranslator.h"

#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>

pqMenuEventTranslator::pqMenuEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

pqMenuEventTranslator::~pqMenuEventTranslator()
{
}

bool pqMenuEventTranslator::translateEvent(QObject* Object, QEvent* Event,
                                           bool& /*Error*/)
{
  QMenu* const menu = qobject_cast<QMenu*>(Object);
  QMenuBar* const menubar = qobject_cast<QMenuBar*>(Object);
  if(!menu && !menubar)
    {
    return false;
    }

  if (menubar)
    {
    QMouseEvent* e = static_cast<QMouseEvent*>(Event);
    if (e->button() == Qt::LeftButton)
      {
      QAction* action = menubar->actionAt(e->pos());
      if (action && action->menu())
        {
        QString which = action->menu()->objectName();
        emit recordEvent(menubar, "activate", which);
        }
      }
    return true;
    }

  if(Event->type() == QEvent::KeyPress)
    {
    QKeyEvent* e = static_cast<QKeyEvent*>(Event);
    if(e->key() == Qt::Key_Enter)
      {
      QAction* action = menu->activeAction();
      if(action)
        {
        QString which = action->objectName();
        if(which == QString::null)
          {
          which = action->text();
          }
        emit recordEvent(menu, "activate", which);
        }
      }
    }
  
  if(Event->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* e = static_cast<QMouseEvent*>(Event);
    if(e->button() == Qt::LeftButton)
      {
      QAction* action = menu->actionAt(e->pos());
      if (action && !action->menu())
        {
        QString which = action->objectName();
        if(which == QString::null)
          {
          which = action->text();
          }
        emit recordEvent(menu, "activate", which);
        }
      }
    }
    
  return true;
}

