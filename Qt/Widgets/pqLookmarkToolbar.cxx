/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkToolbar.cxx

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

========================================================================*/

/// \file pqLookmarkToolbar.cxx
/// \date 7/3/2006

#include "pqLookmarkToolbar.h"

#include <QAction>
#include <QImage>
#include <QIcon>
#include <QPixmap>
#include <QMenu>

#include "pqSetData.h"
#include "pqSetName.h"

//-----------------------------------------------------------------------------
pqLookmarkToolbar::pqLookmarkToolbar(const QString &title, QWidget* p) :  QToolBar(title, p)
{
  this->CurrentLookmark = 0;
  this->setContextMenuPolicy(Qt::CustomContextMenu);

  this->connectActions();
}

//-----------------------------------------------------------------------------
pqLookmarkToolbar::pqLookmarkToolbar(QWidget* p) :  QToolBar(p)
{
  this->CurrentLookmark = 0;
  this->setContextMenuPolicy(Qt::CustomContextMenu);

  this->connectActions();
}

void pqLookmarkToolbar::connectActions()
{
  QObject::connect(this,
      SIGNAL(customContextMenuRequested(const QPoint &)),
      this, SLOT(showContextMenu(const QPoint &)));

  QObject::connect(this, 
    SIGNAL(actionTriggered(QAction*)), SLOT(onLoadLookmark(QAction*)));

  this->ActionEdit = new QAction("Edit",this);
  connect(this->ActionEdit,
    SIGNAL(triggered()), this, SLOT(editCurrentLookmark()));

  this->ActionRemove = new QAction("Delete",this);
  connect(this->ActionRemove,
    SIGNAL(triggered()), this, SLOT(removeCurrentLookmark()));
}

//-----------------------------------------------------------------------------
void pqLookmarkToolbar::onLoadLookmark(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString sourceName = action->data().toString();

  emit this->loadLookmark(sourceName);
}


//-----------------------------------------------------------------------------
void pqLookmarkToolbar::onLookmarkAdded(const QString &name, const QImage &icon)
{
  this->addAction(QIcon(QPixmap::fromImage(icon.scaled(48,48))), name) 
    << pqSetName(name) << pqSetData(name);
}

//-----------------------------------------------------------------------------
void pqLookmarkToolbar::editCurrentLookmark()
{
  if(this->CurrentLookmark)
    {
    emit this->editLookmark(this->CurrentLookmark->text());
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkToolbar::removeCurrentLookmark()
{
  if(this->CurrentLookmark)
    {
    emit this->removeLookmark(this->CurrentLookmark->text());
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkToolbar::showContextMenu(const QPoint &menuPos)
{
  this->CurrentLookmark = this->actionAt(menuPos);
  if(!this->CurrentLookmark)
    {
    return;
    }

  QMenu menu;
  menu.setObjectName("ToolbarLookmarkMenu");

  menu.addAction(this->ActionEdit);
  menu.addAction(this->ActionRemove);

  menu.exec(this->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
void pqLookmarkToolbar::onLookmarkRemoved(const QString &name)
{
  // Remove the action associated with the lookmark.
  QAction *action = this->findChild<QAction *>(name);
  if(action)
    {
    this->removeAction(action);
    delete action;
    }
}

void pqLookmarkToolbar::onLookmarkNameChanged(const QString &oldName, const QString &newName)
{
  QAction *action = this->findChild<QAction *>(oldName);
  if(action)
    {
    action << pqSetName(newName);
    action << pqSetData(newName);
    action->setText(newName);
    action->setIconText(newName);
    action->setToolTip(newName);
    }
}


