/*=========================================================================

   Program: ParaView
   Module:    pqQuickLaunchDialog.cxx

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
#include "pqQuickLaunchDialog.h"
#include "ui_pqQuickLaunchDialog.h"
// Server Manager Includes.

// Qt Includes.
#include <QAction>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QMap>
#include <QPointer>
#include <QStringList>

class pqQuickLaunchDialog::pqInternal : public Ui::QuickLaunchDialog
{
public:
  QMap<QString, QAction*> Actions;
  QMap<QString, QListWidgetItem> Items;
  QString SearchString;
  QPointer<QAction> ActiveAction;
};

//-----------------------------------------------------------------------------
pqQuickLaunchDialog::pqQuickLaunchDialog(QWidget* p):
  Superclass(p, Qt::Dialog|Qt::FramelessWindowHint)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->installEventFilter(this);
  this->Internal->options->installEventFilter(this);

  QObject::connect(this->Internal->options, SIGNAL(currentRowChanged(int)),
    this, SLOT(currentRowChanged(int)));
  this->updateSearch();
}

//-----------------------------------------------------------------------------
pqQuickLaunchDialog::~pqQuickLaunchDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqQuickLaunchDialog::eventFilter (QObject *watched, QEvent *evt)
{
  if (evt->type() == QEvent::KeyPress) 
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);
    int key = keyEvent->key();
    if (key == Qt::Key_Escape)
      {
      if (this->Internal->SearchString.size() > 0)
        {
        this->Internal->SearchString = "";
        this->updateSearch();
        }
      else
        {
        this->reject();
        }
      return true;
      }
    else if (key == Qt::Key_Enter || key == Qt::Key_Return)
      {
      this->accept();
      return true;
      }
    else if ( (key >= Qt::Key_0 && key <= Qt::Key_9) ||
      (key >= Qt::Key_A && key <= Qt::Key_Z) ||
      key == Qt::Key_Space)
      {
      this->Internal->SearchString += keyEvent->text();
      this->updateSearch();
      return true;
      }
    else if (key == Qt::Key_Backspace)
      {
      this->Internal->SearchString.chop(1);
      this->updateSearch();
      return true;
      }
    }

  // pass the event on to the parent class
  return Superclass::eventFilter(watched, evt);
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialog::setActions(const QList<QAction*>& actns)
{
  this->Internal->ActiveAction = 0;
  this->Internal->selection->setText("");
  this->Internal->selection->setIcon(QIcon());
  this->Internal->searchString->setText("( )");
  this->Internal->options->clear();
  this->Internal->SearchString.clear();
  this->Internal->Items.clear();
  this->addActions(actns);
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialog::addActions(const QList<QAction*>& actns)
{
  foreach (QAction* action, actns)
    {
    if (!action->menu())
      {
      QListWidgetItem item(action->icon(), action->text());
      item.setData(Qt::UserRole, action->objectName());
      this->Internal->Items[action->text()] = item;
      this->Internal->Actions[action->objectName()] = action;
      }
    }
}

//-----------------------------------------------------------------------------
// Given the current user selected SearchString, update the GUI.
void pqQuickLaunchDialog::updateSearch()
{
  this->Internal->ActiveAction = 0;
  this->Internal->selection->setText("");
  this->Internal->selection->setIcon(QIcon());
  this->Internal->options->clear();
  this->Internal->searchString->setText(
    QString("( %1 )").arg(this->Internal->SearchString.toLower()));
  if (this->Internal->SearchString.size() == 0)
    {
    return;
    }

  QStringList searchComponents = this->Internal->SearchString.split(" ",
    QString::SkipEmptyParts);

  QStringList searchSpace = this->Internal->Items.keys();

  foreach (QString component, searchComponents)
    {
    searchSpace = searchSpace.filter(component, Qt::CaseInsensitive);
    }

  foreach (QString key, searchSpace)
    {
    QListWidgetItem *item = new QListWidgetItem(this->Internal->Items[key]);
    QString actionName = item->data(Qt::UserRole).toString();
    if (!this->Internal->Actions[actionName]->isEnabled())
      {
      item->setFlags(item->flags()&~Qt::ItemIsEnabled);
      }
    this->Internal->options->addItem(item);
    }
  this->Internal->options->setCurrentRow(0);
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialog::currentRowChanged(int row)
{
  this->Internal->selection->setText("");
  this->Internal->selection->setIcon(QIcon());
  this->Internal->ActiveAction = 0;
  QListWidgetItem* item = this->Internal->options->item(row);
  if (!item)
    {
    return;
    }
  QAction* action = this->Internal->Actions[item->data(Qt::UserRole).toString()]; 
  if (action)
    {
    this->Internal->selection->setText(action->text());
    this->Internal->selection->setIcon(action->icon());
    this->Internal->ActiveAction = action;
    this->Internal->selection->setEnabled(action->isEnabled());
    }
}

//-----------------------------------------------------------------------------
void pqQuickLaunchDialog::accept()
{
  if (this->Internal->ActiveAction && this->Internal->ActiveAction->isEnabled())
    {
    this->Internal->ActiveAction->trigger();
    }
  // Trigger selected action.
  this->Superclass::accept();
}
