/*=========================================================================

   Program: ParaView
   Module:    pqOptionsDialog.cxx

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

/// \file pqOptionsDialog.cxx
/// \date 7/26/2007

#include "pqOptionsDialog.h"
#include "ui_pqOptionsDialog.h"

#include "pqOptionsContainer.h"
#include "pqOptionsPage.h"
#include "pqUndoStack.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QList>
#include <QMap>
#include <QString>


class pqOptionsDialogModelItem
{
public:
  pqOptionsDialogModelItem();
  pqOptionsDialogModelItem(const QString &name);
  ~pqOptionsDialogModelItem();

  pqOptionsDialogModelItem *Parent;
  QString Name;
  QList<pqOptionsDialogModelItem *> Children;
};


class pqOptionsDialogModel : public QAbstractItemModel
{
public:
  pqOptionsDialogModel(QObject *parent=0);
  virtual ~pqOptionsDialogModel();

  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &child) const;

  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  QModelIndex getIndex(const QString &path) const;
  QString getPath(const QModelIndex &index) const;
  void addPath(const QString &path);
  bool removeIndex(const QModelIndex &index);

private:
  QModelIndex getIndex(pqOptionsDialogModelItem *item) const;

private:
  pqOptionsDialogModelItem *Root;
};


class pqOptionsDialogForm : public Ui::pqOptionsFrame
{
public:
  pqOptionsDialogForm();
  ~pqOptionsDialogForm();

  QMap<QString, pqOptionsPage *> Pages;
  pqOptionsDialogModel *Model;
  int ApplyUseCount;
  bool ApplyNeeded;
};


//----------------------------------------------------------------------------
pqOptionsDialogModelItem::pqOptionsDialogModelItem()
  : Name(), Children()
{
  this->Parent = 0;
}

pqOptionsDialogModelItem::pqOptionsDialogModelItem(const QString &name)
  : Name(name), Children()
{
  this->Parent = 0;
}

pqOptionsDialogModelItem::~pqOptionsDialogModelItem()
{
  QList<pqOptionsDialogModelItem *>::Iterator iter = this->Children.begin();
  for( ; iter != this->Children.end(); ++iter)
    {
    delete *iter;
    }
}


//----------------------------------------------------------------------------
pqOptionsDialogModel::pqOptionsDialogModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Root = new pqOptionsDialogModelItem();
}

pqOptionsDialogModel::~pqOptionsDialogModel()
{
  delete this->Root;
}

int pqOptionsDialogModel::rowCount(const QModelIndex &parentIndex) const
{
  pqOptionsDialogModelItem *item = this->Root;
  if(parentIndex.isValid())
    {
    item = reinterpret_cast<pqOptionsDialogModelItem *>(
        parentIndex.internalPointer());
    }

  return item->Children.size();
}

int pqOptionsDialogModel::columnCount(const QModelIndex &) const
{
  return 1;
}

QModelIndex pqOptionsDialogModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  pqOptionsDialogModelItem *item = this->Root;
  if(parentIndex.isValid())
    {
    item = reinterpret_cast<pqOptionsDialogModelItem *>(
        parentIndex.internalPointer());
    }

  if(column == 0 && row >= 0 && row < item->Children.size())
    {
    return this->createIndex(row, column, item->Children[row]);
    }

  return QModelIndex();
}

QModelIndex pqOptionsDialogModel::parent(const QModelIndex &child) const
{
  if(child.isValid())
    {
    pqOptionsDialogModelItem *item =
        reinterpret_cast<pqOptionsDialogModelItem *>(child.internalPointer());
    return this->getIndex(item->Parent);
    }

  return QModelIndex();
}

QVariant pqOptionsDialogModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid())
    {
    pqOptionsDialogModelItem *item =
        reinterpret_cast<pqOptionsDialogModelItem *>(idx.internalPointer());
    if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
      {
      return QVariant(item->Name);
      }
    }

  return QVariant();
}

QModelIndex pqOptionsDialogModel::getIndex(const QString &path) const
{
  pqOptionsDialogModelItem *item = this->Root;
  QStringList names = path.split(".");
  QStringList::Iterator iter = names.begin();
  for( ; item && iter != names.end(); ++iter)
    {
    pqOptionsDialogModelItem *child = 0;
    QList<pqOptionsDialogModelItem *>::Iterator jter = item->Children.begin();
    for( ; jter != item->Children.end(); ++jter)
      {
      if((*jter)->Name == *iter)
        {
        child = *jter;
        break;
        }
      }

    item = child;
    }

  if(item && item != this->Root)
    {
    return this->getIndex(item);
    }

  return QModelIndex();
}

QString pqOptionsDialogModel::getPath(const QModelIndex &idx) const
{
  if(idx.isValid())
    {
    QString path;
    pqOptionsDialogModelItem *item =
        reinterpret_cast<pqOptionsDialogModelItem *>(idx.internalPointer());
    if(item)
      {
      path = item->Name;
      item = item->Parent;
      }

    while(item && item != this->Root)
      {
      path.prepend(".").prepend(item->Name);
      item = item->Parent;
      }

    return path;
    }

  return QString();
}

void pqOptionsDialogModel::addPath(const QString &path)
{
  pqOptionsDialogModelItem *item = this->Root;
  QStringList names = path.split(".");
  QStringList::Iterator iter = names.begin();
  for( ; iter != names.end(); ++iter)
    {
    pqOptionsDialogModelItem *child = 0;
    QList<pqOptionsDialogModelItem *>::Iterator jter = item->Children.begin();
    for( ; jter != item->Children.end(); ++jter)
      {
      if((*jter)->Name == *iter)
        {
        child = *jter;
        break;
        }
      }

    if(!child)
      {
      child = new pqOptionsDialogModelItem(*iter);
      child->Parent = item;
      QModelIndex parentIndex = this->getIndex(item);
      int row = item->Children.size();
      this->beginInsertRows(parentIndex, row, row);
      item->Children.append(child);
      this->endInsertRows();
      }

    item = child;
    }
}

bool pqOptionsDialogModel::removeIndex(const QModelIndex &idx)
{
  if(idx.isValid())
    {
    pqOptionsDialogModelItem *item =
        reinterpret_cast<pqOptionsDialogModelItem *>(idx.internalPointer());
    if(item->Children.size() == 0)
      {
      QModelIndex parentIndex = this->getIndex(item->Parent);
      this->beginRemoveRows(parentIndex, idx.row(), idx.row());
      item->Parent->Children.removeAt(idx.row());
      this->endRemoveRows();
      delete item;
      return true;
      }
    }

  return false;
}

QModelIndex pqOptionsDialogModel::getIndex(
    pqOptionsDialogModelItem *item) const
{
  if(item && item->Parent)
    {
    return this->createIndex(item->Parent->Children.indexOf(item), 0, item);
    }

  return QModelIndex();
}


//----------------------------------------------------------------------------
pqOptionsDialogForm::pqOptionsDialogForm()
  : Ui::pqOptionsFrame(), Pages()
{
  this->Model = new pqOptionsDialogModel();
  this->ApplyUseCount = 0;
  this->ApplyNeeded = false;
}

pqOptionsDialogForm::~pqOptionsDialogForm()
{
  delete this->Model;
}


//----------------------------------------------------------------------------
pqOptionsDialog::pqOptionsDialog(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Form = new pqOptionsDialogForm();
  this->Form->setupUi(this);
  this->Form->PageNames->setModel(this->Form->Model);

  // Hide the tree widget header view.
  this->Form->PageNames->header()->hide();

  // Hide the apply and reset buttons until they are needed.
  this->Form->ApplyButton->setEnabled(false);
  this->Form->ResetButton->setEnabled(false);
  this->Form->ApplyButton->hide();
  this->Form->ResetButton->hide();

  this->connect(this->Form->PageNames->selectionModel(),
      SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
      this, SLOT(changeCurrentPage()));
  this->connect(this->Form->ApplyButton, SIGNAL(clicked()),
      this, SLOT(applyChanges()));
  this->connect(this->Form->ResetButton, SIGNAL(clicked()),
      this, SLOT(resetChanges()));
  this->connect(this->Form->CloseButton, SIGNAL(clicked()),
      this, SLOT(accept()));
}

pqOptionsDialog::~pqOptionsDialog()
{
  delete this->Form;
}

bool pqOptionsDialog::isApplyNeeded() const
{
  return this->Form->ApplyNeeded;
}

void pqOptionsDialog::setApplyNeeded(bool applyNeeded)
{
  if(applyNeeded != this->Form->ApplyNeeded)
    {
    if(!applyNeeded)
      {
      this->Form->ApplyNeeded = false;
      this->Form->ApplyButton->setEnabled(false);
      this->Form->ResetButton->setEnabled(false);
      }
    else if(this->Form->ApplyUseCount > 0)
      {
      this->Form->ApplyNeeded = true;
      this->Form->ApplyButton->setEnabled(true);
      this->Form->ResetButton->setEnabled(true);
      }
    }
}

void pqOptionsDialog::addOptions(const QString &path, pqOptionsPage *options)
{
  if(!options)
    {
    return;
    }

  // See if the page is a container.
  pqOptionsContainer *container = qobject_cast<pqOptionsContainer *>(options);
  if(!container && path.isEmpty())
    {
    return;
    }

  // See if the page/container uses the apply button.
  if(options->isApplyUsed())
    {
    this->Form->ApplyUseCount++;
    if(this->Form->ApplyUseCount == 1)
      {
      this->Form->ApplyButton->show();
      this->Form->ResetButton->show();
      QObject::connect(this, SIGNAL(accepted()), this, SLOT(applyChanges()));
      }

    this->connect(options, SIGNAL(changesAvailable()),
        this, SLOT(enableButtons()));
    }

  // Add the widget to the stack.
  this->Form->Stack->addWidget(options);

  // Add the page(s) to the map and the model.
  if(container)
    {
    // If the path is not empty, use it as the page prefix.
    QString prefix;
    if(!path.isEmpty())
      {
      prefix = path;
      prefix.append(".");
      }

    container->setPagePrefix(prefix);

    // Get the list of pages from the container.
    QStringList pathList = container->getPageList();
    QStringList::Iterator iter = pathList.begin();
    for( ; iter != pathList.end(); ++iter)
      {
      this->Form->Pages.insert(prefix + *iter, options);
      this->Form->Model->addPath(prefix + *iter);
      }
    }
  else
    {
    this->Form->Pages.insert(path, options);
    this->Form->Model->addPath(path);
    }
}

void pqOptionsDialog::addOptions(pqOptionsContainer *options)
{
  this->addOptions(QString(), options);
}

void pqOptionsDialog::removeOptions(pqOptionsPage *options)
{
  if(!options)
    {
    return;
    }

  // Remove the widget from the stack.
  this->Form->Stack->removeWidget(options);

  // See if the options use the apply button.
  if(options->isApplyUsed())
    {
    this->Form->ApplyUseCount--;
    if(this->Form->ApplyUseCount == 0)
      {
      this->Form->ApplyNeeded = false;
      this->Form->ApplyButton->setEnabled(false);
      this->Form->ResetButton->setEnabled(false);
      this->Form->ApplyButton->hide();
      this->Form->ResetButton->hide();
      QObject::disconnect(this, SIGNAL(accepted()), this, SLOT(applyChanges()));
      }

    this->disconnect(options, 0, this, 0);
    }

  // Search the map for the paths to remove.
  QMap<QString, pqOptionsPage *>::Iterator iter = this->Form->Pages.begin();
  while(iter != this->Form->Pages.end())
    {
    if(*iter == options)
      {
      QString path = iter.key();
      iter = this->Form->Pages.erase(iter);

      // Remove the item from the tree model as well.
      QModelIndex index = this->Form->Model->getIndex(path);
      QPersistentModelIndex parentIndex = index.parent();
      if(this->Form->Model->removeIndex(index))
        {
        // Remove any empty parent items.
        while(parentIndex.isValid() &&
            this->Form->Model->rowCount(parentIndex) == 0)
          {
          index = parentIndex;
          parentIndex = index.parent();

          // Make sure the index path isn't in the map.
          path = this->Form->Model->getPath(index);
          if(this->Form->Pages.find(path) == this->Form->Pages.end())
            {
            if(!this->Form->Model->removeIndex(index))
              {
              break;
              }
            }
          }
        }
      }
    else
      {
      ++iter;
      }
    }
}

void pqOptionsDialog::setCurrentPage(const QString &path)
{
  QModelIndex current = this->Form->Model->getIndex(path);
  this->Form->PageNames->setCurrentIndex(current);
}

void pqOptionsDialog::applyChanges()
{
  if(this->Form->ApplyNeeded)
    {
    BEGIN_UNDO_SET("Changed View Settings");
    emit this->aboutToApplyChanges();
    QMap<QString, pqOptionsPage *>::Iterator iter = this->Form->Pages.begin();
    for( ; iter != this->Form->Pages.end(); ++iter)
      {
      (*iter)->applyChanges();
      }

    this->setApplyNeeded(false);
    emit this->appliedChanges();
    END_UNDO_SET();
    }
}

void pqOptionsDialog::resetChanges()
{
  if(this->Form->ApplyNeeded)
    {
    QMap<QString, pqOptionsPage *>::Iterator iter = this->Form->Pages.begin();
    for( ; iter != this->Form->Pages.end(); ++iter)
      {
      (*iter)->resetChanges();
      }

    this->setApplyNeeded(false);
    }
}

void pqOptionsDialog::changeCurrentPage()
{
  // Get the current index from the view.
  QModelIndex current = this->Form->PageNames->currentIndex();

  // Look up the path for the current index.
  QString path = this->Form->Model->getPath(current);
  QMap<QString, pqOptionsPage *>::Iterator iter = this->Form->Pages.find(path);
  if(iter == this->Form->Pages.end())
    {
    // If no page is found, show the blank page.
    this->Form->Stack->setCurrentWidget(this->Form->BlankPage);
    }
  else
    {
    this->Form->Stack->setCurrentWidget(*iter);
    pqOptionsContainer *container = qobject_cast<pqOptionsContainer *>(*iter);
    if(container)
      {
      // Get the path prefix from the container.
      QString prefix = container->getPagePrefix();
      if(!prefix.isEmpty())
        {
        // Remove the prefix from the path.
        path.remove(0, prefix.length());
        }

      // Set the page on the container object.
      container->setPage(path);
      }
    }
}

void pqOptionsDialog::enableButtons()
{
  this->setApplyNeeded(true);
}


