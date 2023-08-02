// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqQuickLaunchDialog.h"
#include "pqQtDeprecated.h"
#include "ui_pqQuickLaunchDialog.h"
// Server Manager Includes.

// Qt Includes.
#include <QAction>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QMap>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
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
  bool QuickApply = false;
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
  Q_FOREACH (const QStringList& exp, searchExpressions)
  {
    QString part = exp.join("\\w*\\W+");
    QRegularExpression regExp("^" + part, QRegularExpression::CaseInsensitiveOption);
    searchSpace += keys.filter(regExp);
  }

  // Now build up the list of matches to the search components disregarding
  // word order and proximity entirely.
  // (BUG #0016116).
  QStringList filteredkeys = keys;
  Q_FOREACH (const QString& component, searchComponents)
  {
    filteredkeys =
      filteredkeys.filter(QRegularExpression(component, QRegularExpression::CaseInsensitiveOption));
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
  this->Internal->label->setText(
    QString("<html><head/><body><p>%1 <span style=\" font-weight:600; font-style:italic;\"> %2 "
            "</span> %3 <span style=\" font-weight:600; font-style:italic;\"> %4 </span> %5 <span "
            "style=\" font-weight:600; font-style:italic;\"> %6 </span> %7 </p></body></html>")
      .arg(tr("Type to search."))
      .arg(tr("Enter"))
      .arg(tr("to create selected source/filter."))
      .arg(tr("Shift + Enter"))
      .arg(tr("to create and apply selected source/filter. "))
      .arg(tr("Esc"))
      .arg(tr("to cancel.")));
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
  this->Internal->ActiveAction = nullptr;
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
  Q_FOREACH (QAction* action, actns)
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
  this->Internal->ActiveAction = nullptr;
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
    this->Internal->SearchString.split(" ", PV_QT_SKIP_EMPTY_PARTS);

  QList<QStringList> searchExpressions;
  fillPermutations(searchExpressions, searchComponents);

  QStringList searchSpace;
  fillSearchSpace(searchSpace, searchComponents, searchExpressions, keys);

  QStringList fuzzySearchComponents;
  Q_FOREACH (const QString& word, searchComponents)
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
  Q_FOREACH (QString key, searchSpace)
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
  this->Internal->ActiveAction = nullptr;
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
  this->Internal->QuickApply = false;
  if (this->Internal->ActiveAction && this->Internal->ActiveAction->isEnabled())
  {
    this->Internal->ActiveAction->trigger();
    this->Internal->QuickApply = QApplication::keyboardModifiers() & Qt::ShiftModifier;
  }
  // Trigger selected action.
  this->Superclass::accept();
}

//-----------------------------------------------------------------------------
bool pqQuickLaunchDialog::quickApply()
{
  return this->Internal->QuickApply;
}
