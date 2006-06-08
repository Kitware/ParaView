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

bool pqAbstractActivateEventPlayer::playEvent(QObject* Object,
                                              const QString& Command,
                                              const QString& Arguments,
                                              bool& Error)
{
  if(Command != "activate")
    return false;

  if(QMenu* const object = qobject_cast<QMenu*>(Object))
    {
    
    QAction* action = NULL;
    QList<QAction*> actions = object->actions();
    for(int j = 0; j != actions.size() && !action; ++j)
      {
      if(actions[j]->objectName() == Arguments)
        {
        action = actions[j];
        }
      }

    if(!action)
      {
      qCritical() << "couldn't find action " << Arguments;
      Error = true;
      return true;
      }

    // get a list of menus that must be navigated to 
    // click on the action
    QObjectList menus;
    for(QObject* p = object; 
        qobject_cast<QMenu*>(p) || qobject_cast<QMenuBar*>(p); 
        p = p->parent())
      {
      menus.push_front(p);
      }

    // unfold menus to make action visible
    int i;
    int numMenus = menus.size() - 1;
    for(i=0; i < numMenus; ++i)
      {
      QObject* p = menus[i];
      QMenu* next = qobject_cast<QMenu*>(menus[i+1]);
      if(QMenuBar* menu_bar = qobject_cast<QMenuBar*>(p))
        {
        menu_bar->setActiveAction(next->menuAction());
        while(!next->isVisible())
          pqTesting::NonBlockingSleep(100);
        }
      else if(QMenu* menu = qobject_cast<QMenu*>(p))
        {
        QRect geom = menu->actionGeometry(next->menuAction());
        QMouseEvent button_press(QEvent::MouseButtonPress, 
                                 geom.center(), 
                                 Qt::LeftButton, Qt::LeftButton, 0);
        QApplication::sendEvent(menu, &button_press);
        // process events before mouse up to help our sub menu show up
        QCoreApplication::processEvents();  
        
        QMouseEvent button_release(QEvent::MouseButtonRelease, 
                                   geom.center(), 
                                   Qt::LeftButton, 0, 0);
        QApplication::sendEvent(menu, &button_release);
        
        while(!next->isVisible())
          pqTesting::NonBlockingSleep(100);
        }
      }
    
    // simulate mouse click on menu item
    QRect geom = object->actionGeometry(action);
    QMouseEvent button_press(QEvent::MouseButtonPress, 
                             geom.center(), 
                             Qt::LeftButton, Qt::LeftButton, 0);
    QApplication::sendEvent(object, &button_press);
    
    QMouseEvent button_release(QEvent::MouseButtonRelease, 
                               geom.center(), 
                               Qt::LeftButton, 0, 0);
    QApplication::sendEvent(object, &button_release);

    // TODO:  If mouse happens to be over where a menu shows up
    //        the mouse messes up the activation of the menu item
    
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
