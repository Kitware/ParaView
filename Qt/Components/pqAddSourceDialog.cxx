/*=========================================================================

   Program: ParaView
   Module:    pqAddSourceDialog.cxx

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

/// \file pqAddSourceDialog.cxx
/// \date 5/26/2006

#include "pqAddSourceDialog.h"
#include "ui_pqAddSourceDialog.h"
#include "pqSourceInfoGroupMap.h"
#include "pqSourceInfoModel.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QEvent>
#include <QKeyEvent>
#include <QList>
#include <QMessageBox>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QString>
#include <QStringList>


class pqAddSourceDialogForm : public Ui::pqAddSourceDialog
{
public:
  QList<QPersistentModelIndex> BackHistory;
};


//-----------------------------------------------------------------------------
pqAddSourceDialog::pqAddSourceDialog(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Sources = 0;
  this->History = 0;
  this->SourceInfo = 0;
  this->Groups = 0;
  this->Form = new pqAddSourceDialogForm();
  this->Form->setupUi(this);

  // Disable the 'new group' and 'add to favorites' buttons until the
  // source group map is set.
  this->Form->AddToFavoritesButton->setEnabled(false);
  this->Form->NewFolderButton->setEnabled(false);

  // Set up the change root functionality.
  QObject::connect(this->Form->SourceList,
      SIGNAL(activated(const QModelIndex &)),
      this, SLOT(changeRoot(const QModelIndex &)));
  QObject::connect(this->Form->SourceGroup, SIGNAL(activated(int)),
      this, SLOT(changeRoot(int)));

  // Listen for history activation events.
  QObject::connect(this->Form->SourceHistory,
      SIGNAL(activated(const QModelIndex &)),
      this, SLOT(activateHistoryIndex(const QModelIndex &)));

  // Listen to the button press events.
  QObject::connect(this->Form->OkButton, SIGNAL(clicked()),
      this, SLOT(validateChoice()));
  QObject::connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));
  QObject::connect(this->Form->BackButton, SIGNAL(clicked()),
      this, SLOT(navigateBack()));
  QObject::connect(this->Form->UpButton, SIGNAL(clicked()),
      this, SLOT(navigateUp()));
  QObject::connect(this->Form->NewFolderButton, SIGNAL(clicked()),
      this, SLOT(addFolder()));
  QObject::connect(this->Form->AddToFavoritesButton, SIGNAL(clicked()),
      this, SLOT(addFavorite()));

  // Listen for the delete key.
  this->Form->SourceList->installEventFilter(this);
}

pqAddSourceDialog::~pqAddSourceDialog()
{
  if(this->Form)
    {
    delete this->Form;
    }
}

bool pqAddSourceDialog::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->Form->SourceList && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete)
      {
      this->deleteSelected();
      }
    }

  return QWidget::eventFilter(object, e);
}

void pqAddSourceDialog::setSourceLabel(const QString &label)
{
  this->Form->SourceLabel->setText(label);
  QString sourceLabel = label + " Group";
  this->Form->SourceGroupLabel->setText(sourceLabel);
}

void pqAddSourceDialog::setSourceMap(pqSourceInfoGroupMap *groups)
{
  this->Groups = groups;

  // TODO: Enable the 'add group' button.
  //this->Form->NewFolderButton->setEnabled(true);
}

void pqAddSourceDialog::setSourceList(QAbstractItemModel *sources)
{
  if(sources != this->Sources)
    {
    this->Sources = sources;
    this->Form->SourceList->setModel(this->Sources);

    // Check for a source info model and save the pointer. The source
    // info model can be used to determine if the selected item is a
    // folder or source. Skip all the proxy models to find the 'real'
    // model.
    QAbstractItemModel *model = this->Sources;
    while(model)
      {
      QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(model);
      if(proxy)
        {
        model = proxy->sourceModel();
        }
      else
        {
        break;
        }
      }

    this->SourceInfo = qobject_cast<pqSourceInfoModel *>(model);

    // Set up the path in the combobox.
    this->Form->SourceGroup->clear();
    this->Form->SourceGroup->addItem(QIcon(":/pqWidgets/Icons/pqFolder16.png"), "Filters");
    this->Form->SourceGroup->setCurrentIndex(0);

    // Reset the back button history.
    this->Form->BackHistory.clear();
    this->Form->BackHistory.append(QPersistentModelIndex());
    this->Form->BackButton->setEnabled(false);

    // Listen to the selection events from the view.
    QItemSelectionModel *selection = this->Form->SourceList->selectionModel();
    QObject::connect(selection,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(updateFromSources(const QModelIndex &, const QModelIndex &)));
    }
}

void pqAddSourceDialog::setHistoryList(QAbstractItemModel *history)
{
  if(history != this->History)
    {
    this->History = history;
    this->Form->SourceHistory->setModel(this->History);

    // Listen to the selection events from the view.
    QItemSelectionModel *selection = this->Form->SourceHistory->selectionModel();
    QObject::connect(selection,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(updateFromHistory(const QModelIndex &, const QModelIndex &)));
    }
}

void pqAddSourceDialog::getPath(QString &path)
{
  QStringList groups;
  this->getPath(this->Form->SourceList->rootIndex(), groups);
  path = groups.join("/");
}

void pqAddSourceDialog::setPath(const QString &path)
{
  if(!this->Sources)
    {
    return;
    }

  // Split the path apart. Search the model hierarchy for the index
  // with the specified path. Set as much of the path as possible.
  int row = 0;
  int rows = 0;
  QString indexName;
  QModelIndex index;
  QModelIndex rootIndex;
  QStringList groups = path.split("/");
  QStringList::Iterator iter = groups.begin();
  for( ; iter != groups.end(); ++iter)
    {
    // Search the child indexes for the path name.
    rows = this->Sources->rowCount(rootIndex);
    for(row = 0; row < rows; row++)
      {
      index = this->Sources->index(row, 0, rootIndex);
      indexName = this->Sources->data(index, Qt::DisplayRole).toString();
      if(indexName == *iter)
        {
        rootIndex = index;
        break;
        }
      }

    if(row >= rows)
      {
      break;
      }
    }

  if(rootIndex != this->Form->SourceList->rootIndex())
    {
    this->changeRoot(rootIndex);

    // This should be the starting root index, so clear the previous
    // history.
    if(this->Form->BackHistory.size() == 2)
      {
      this->Form->BackHistory.removeFirst();
      this->Form->BackButton->setEnabled(false);
      }
    }
}

void pqAddSourceDialog::getSource(QString &name)
{
  name = this->Form->SourceName->text();
}

void pqAddSourceDialog::setSource(const QString &name)
{
  this->Form->SourceName->setText(name);
}

void pqAddSourceDialog::navigateBack()
{
  if(this->Form->BackHistory.size() > 1)
    {
    // Take the last index off the list. Then, set the previous index
    // as the root index.
    this->Form->BackHistory.removeLast();
    this->changeRoot(this->Form->BackHistory.last());

    // Remove the duplicate added when setting the root index.
    this->Form->BackHistory.removeLast();

    // Disable the button if the list has only one item.
    this->Form->BackButton->setEnabled(this->Form->BackHistory.size() > 1);
    }
}

void pqAddSourceDialog::navigateUp()
{
  if(this->Form->SourceGroup->count() > 1)
    {
    int newIndex = this->Form->SourceGroup->count() - 2;
    this->Form->SourceGroup->setCurrentIndex(newIndex);
    this->changeRoot(newIndex);
    }
}

void pqAddSourceDialog::addFolder()
{
  // TODO
}

void pqAddSourceDialog::addFavorite()
{
  if(!this->Groups)
    {
    return;
    }

  // Get the selected source from the model.
  QModelIndex index = this->Form->SourceList->currentIndex();
  if(index.isValid() && this->isModelSource(index))
    {
    QString name = this->Sources->data(index, Qt::DisplayRole).toString();
    this->Groups->addSource(name, "Favorites");
    }
}

void pqAddSourceDialog::deleteSelected()
{
  if(!this->Groups || !this->SourceInfo)
    {
    return;
    }

  // Get the selected source from the model.
  QModelIndex index = this->Form->SourceList->currentIndex();
  if(index.isValid())
    {
    bool isSource = this->isModelSource(index);
    QString name = this->Sources->data(index, Qt::DisplayRole).toString();
    if(name.isEmpty())
      {
      return;
      }

    // The sources in the root index cannot be deleted. The 'Favorites'
    // group cannot be deleted either.
    if(this->Form->SourceList->rootIndex() == QModelIndex())
      {
      if(isSource)
        {
        QString type = this->Form->SourceLabel->text().toLower();
        QString text = "The selected " + type;
        text += " cannot be deleted.\nOnly " + type + "s in groups can "
                "be deleted.";
        QMessageBox::warning(this, "Delete Error", text,
            QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
        return;
        }
      else if(name == "Favorites")
        {
        QMessageBox::warning(this, "Delete Error",
            "The \"Favorites\" group cannot be deleted.",
            QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
        return;
        }
      }

    // Remove the source or group from the source map.
    QStringList path;
    this->getPath(this->Form->SourceList->rootIndex(), path);
    if(isSource)
      {
      this->Groups->removeSource(name, path.join("/"));
      }
    else
      {
      path.append(name);
      this->Groups->removeGroup(path.join("/"));
      }
    }
}

void pqAddSourceDialog::validateChoice()
{
  if(!this->SourceInfo)
    {
    this->accept();
    return;
    }

  // Make sure the filter entered is valid.
  QString choice = this->Form->SourceName->text();
  if(choice.isEmpty())
    {
    QString type = this->Form->SourceLabel->text().toLower();
    QString text = "The " + type;
    text += " name is empty.\nPlease select or enter a " + type + ".";
    QMessageBox::warning(this, "No Selection", text,
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    }
  else if(!this->SourceInfo->isSource(choice))
    {
    QString type = this->Form->SourceLabel->text().toLower();
    QString title = "Invalid " + this->Form->SourceLabel->text();
    QString text = "The selected " + type;
    text += " does not exist.\nPlease select or enter a valid " + type + ".";
    QMessageBox::warning(this, title, text,
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    }
  else
    {
    this->accept();
    }
}

void pqAddSourceDialog::changeRoot(const QModelIndex &index)
{
  // Check the index. If it is a source, activating it should be
  // the same thing as hitting the ok button.
  if(this->isModelSource(index))
    {
    this->accept();
    return;
    }

  // Change the root index for the source list.
  this->Form->SourceList->setRootIndex(index);

  // Update the back button history.
  this->Form->BackHistory.append(index);
  this->Form->BackButton->setEnabled(true);

  // Get the path for the specified index. Use the path list to set up
  // the group combobox.
  QStringList path;
  this->getPath(index, path);
  path.prepend("Filters");
  this->Form->SourceGroup->clear();
  QIcon folder = QIcon(":/pqWidgets/Icons/pqFolder16.png");
  QStringList::Iterator iter = path.begin();
  for( ; iter != path.end(); ++iter)
    {
    this->Form->SourceGroup->addItem(folder, *iter);
    }

  // Select the last path name added as the current group.
  this->Form->SourceGroup->setCurrentIndex(
      this->Form->SourceGroup->count() - 1);
}

void pqAddSourceDialog::changeRoot(int index)
{
  if(!this->Sources || index == this->Form->SourceGroup->count() - 1)
    {
    return;
    }

  // The last index in the combobox represents the current root index
  // int the source list. Change the root index to the one pointed to
  // by the selected combobox index. Remove the combobox items after
  // the selected index as well.
  QModelIndex rootIndex = this->Form->SourceList->rootIndex();
  for(int i = this->Form->SourceGroup->count() - 1; i > index; i--)
    {
    rootIndex = this->Sources->parent(rootIndex);
    this->Form->SourceGroup->removeItem(i);
    }

  // Change the root index for the source list.
  this->Form->SourceList->setRootIndex(rootIndex);

  // Update the back button history.
  this->Form->BackHistory.append(rootIndex);
  this->Form->BackButton->setEnabled(true);
}

void pqAddSourceDialog::updateFromSources(const QModelIndex &current,
    const QModelIndex &)
{
  if(this->Sources)
    {
    // Get the name for the selected index. Set the name when the
    // selected item is a source. Clear the name when the selected
    // item is a folder.
    QString name;
    if(current.isValid() && this->isModelSource(current))
      {
      name = this->Sources->data(current, Qt::DisplayRole).toString();
      this->Form->AddToFavoritesButton->setEnabled(this->Groups != 0);
      }
    else
      {
      this->Form->AddToFavoritesButton->setEnabled(false);
      }

    // Set the selected name as the text in the line edit.
    this->setSource(name);
    }
}

void pqAddSourceDialog::updateFromHistory(const QModelIndex &current,
    const QModelIndex &)
{
  if(this->History && current.isValid())
    {
    QString name = this->History->data(current, Qt::DisplayRole).toString();
    this->setSource(name);
    }
}

void pqAddSourceDialog::activateHistoryIndex(const QModelIndex &/*index*/)
{
  this->accept();
}

void pqAddSourceDialog::getPath(const QModelIndex &index, QStringList &path)
{
  QModelIndex current = index;
  while(current.isValid())
    {
    path.prepend(this->Sources->data(current, Qt::DisplayRole).toString());
    current = this->Sources->parent(current);
    }
}

bool pqAddSourceDialog::isModelSource(const QModelIndex &index) const
{
  if(!this->SourceInfo)
    {
    return true;
    }

  // If using proxy models, get the index for the source info model.
  QModelIndex idx = index;
  QAbstractItemModel *model = this->Sources;
  while(model)
    {
    QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(model);
    if(proxy)
      {
      idx = proxy->mapToSource(idx);
      model = proxy->sourceModel();
      }
    else
      {
      break;
      }
    }

  // The index should be in source coordinates now.
  return this->SourceInfo->isSource(idx);
}



