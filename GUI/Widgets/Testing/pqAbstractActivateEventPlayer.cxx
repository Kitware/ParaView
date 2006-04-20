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
#include <QMenuBar>
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
    QObjectList parents;
    for(QObject* p = object->parent(); p; p = p->parent())
      {
      parents.push_front(p);
      }
      
    for(int i = 0; i != parents.size(); ++i)
      {
      if(QMenuBar* const menu_bar = qobject_cast<QMenuBar*>(parents[i]))
        {
        if(QMenu* const menu = (i+1) < parents.size() ? qobject_cast<QMenu*>(parents[i+1]) : 0)
          {
          QList<QAction*> actions = menu_bar->actions();
          for(int j = 0; j != actions.size(); ++j)
            {
            if(actions[j]->menu() == menu)
              {
              menu_bar->setActiveAction(actions[j]);
              while(!menu->isVisible())
                pqTesting::NonBlockingSleep(100);
              break;
              }
            }
          }
        }
      else if(QMenu* const menu = qobject_cast<QMenu*>(parents[i]))
        {
        if((i+1) < parents.size())
          {
          if(QMenu* const submenu = (i+1) < parents.size() ? qobject_cast<QMenu*>(parents[i+1]) : 0)
            {
            const QString temp = submenu->objectName();
            
            QList<QAction*> actions = menu->actions();
            for(int j = 0; j != actions.size(); ++j)
              {
              const QString temp2 = actions[j]->menu() ? actions[j]->menu()->objectName() : "";
              
              if(actions[j]->menu() == submenu)
                {
                QMouseEvent button_press(QEvent::MouseButtonPress, menu->actionGeometry(actions[j]).center(), Qt::LeftButton, Qt::LeftButton, 0);
                QApplication::sendEvent(menu, &button_press);
                
                QMouseEvent button_release(QEvent::MouseButtonRelease, menu->actionGeometry(actions[j]).center(), Qt::LeftButton, 0, 0);
                QApplication::sendEvent(menu, &button_release);

                while(!submenu->isVisible())
                  pqTesting::NonBlockingSleep(100);
                break;
                }
              }
            }
          }
        else
          {
            QList<QAction*> actions = menu->actions();
            for(int j = 0; j != actions.size(); ++j)
              {
              if(actions[j] == object)
                {
                menu->setActiveAction(actions[j]);
                
                QMouseEvent button_press(QEvent::MouseButtonPress, menu->actionGeometry(actions[j]).center(), Qt::LeftButton, Qt::LeftButton, 0);
                QApplication::sendEvent(menu, &button_press);
                
                QMouseEvent button_release(QEvent::MouseButtonRelease, menu->actionGeometry(actions[j]).center(), Qt::LeftButton, 0, 0);
                QApplication::sendEvent(menu, &button_release);
                return true;
                }
              }
            }
          }
        }
/*
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
      
      menu->setActiveAction(object);
      }
*/
      
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
