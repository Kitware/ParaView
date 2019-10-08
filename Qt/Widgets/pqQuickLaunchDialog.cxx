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
#include <QPushButton>
#include <QStringList>
#include <QtDebug>
#include <algorithm>

class pqQuickLaunchDialog::pqInternal : public Ui::QuickLaunchDialog
{
public:
  QMap<QString, QAction*> Actions;
  QMap<QString, QListWidgetItem> Items;
  QString SearchString;
  QPointer<QAction> ActiveAction;
};

namespace
{
void fillPermutations(QList<QStringList>& list, QStringList words)
{
  list.push_back(words);
  words.sort();
  do
  {
    list.push_back(words);
  } while (std::next_permutation(words.begin(), words.end()));
}

void fillSearchSpace(QStringList& searchSpace, const QStringList& searchComponents,
  const QList<QStringList>& searchExpressions, const QStringList& keys)
{
  foreach (const QStringList& exp, searchExpressions)
  {
    QString part = exp.join("\\w*\\W+");
    QRegExp regExp("^" + part, Qt::CaseInsensitive);
    searchSpace += keys.filter(regExp);
  }

  // Now build up the list of matches to the search components disregarding
  // word order and proximity entirely.
  // (BUG #0016116).
  QStringList filteredkeys = keys;
  foreach (const QString& component, searchComponents)
  {
    filteredkeys = filteredkeys.filter(QRegExp(component, Qt::CaseInsensitive));
  }
  searchSpace += filteredkeys;
}
}

//-----------------------------------------------------------------------------
pqQuickLaunchDialog::pqQuickLaunchDialog(QWidget* p)
  : Superclass(p, Qt::Dialog | Qt::FramelessWindowHint)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->installEventFilter(this);
  this->Internal->options->installEventFilter(this);

  QObject::connect(
    this->Internal->options, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChanged(int)));

  QObject::connect(this->Internal->selection, SIGNAL(clicked()), this, SLOT(accept()));

  this->updateSearch();
}

//-----------------------------------------------------------------------------
pqQuickLaunchDialog::~pqQuickLaunchDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqQuickLaunchDialog::eventFilter(QObject* watched, QEvent* evt)
{
  if (evt->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
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
    else if ((key >= Qt::Key_0 && key <= Qt::Key_9) || (key >= Qt::Key_A && key <= Qt::Key_Z) ||
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
      QString nameWithoutShortcut = action->text().remove('&');
      QListWidgetItem item(action->icon(), nameWithoutShortcut);
      item.setData(Qt::UserRole, action->objectName());
      this->Internal->Items[nameWithoutShortcut] = item;
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

  const QStringList keys = this->Internal->Items.keys();
  const QStringList searchComponents =
    this->Internal->SearchString.split(" ", QString::SkipEmptyParts);

  QList<QStringList> searchExpressions;
  fillPermutations(searchExpressions, searchComponents);

  QStringList searchSpace;
  fillSearchSpace(searchSpace, searchComponents, searchExpressions, keys);

  QStringList fuzzySearchComponents;
  foreach (const QString& word, searchComponents)
  {
    QString newword;
    for (int cc = 0; cc < word.size(); cc++)
    {
      newword += word[cc];
      newword += "\\w*";
    }
    fuzzySearchComponents.push_back(newword);
  }

  searchExpressions.clear();
  fillPermutations(searchExpressions, fuzzySearchComponents);
  fillSearchSpace(searchSpace, fuzzySearchComponents, searchExpressions, keys);

  searchSpace.removeDuplicates();

  int currentRow = -1;
  int i = 0;
  foreach (QString key, searchSpace)
  {
    QListWidgetItem* item = new QListWidgetItem(this->Internal->Items[key]);
    QString actionName = item->data(Qt::UserRole).toString();
    if (!this->Internal->Actions[actionName]->isEnabled())
    {
      item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    else
    {
      if (currentRow == -1)
      {
        currentRow = i;
      }
    }
    this->Internal->options->addItem(item);
    i++;
  }
  this->Internal->options->setCurrentRow(currentRow);
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
